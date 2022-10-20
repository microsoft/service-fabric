/*++

Copyright (c) Microsoft Corporation

Module Name:

    KHashTableTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KHashTableTests.h.
    2. Add an entry to the gs_KuTestCases table in KHashTableTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.



--*/

#define KTL_HASH_STATS

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KHashTableTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* gp_Allocator = 0;


ULONG MyHashFunction(const ULONG& Val)
{
    return ~Val;
}

NTSTATUS
Test1()
{
    KHashTable<ULONG, KWString> Tbl(121, MyHashFunction, *gp_Allocator);

    NTSTATUS Result;

    ULONG Key = 32;
    Result =  Tbl.Put(Key, KWString(KtlSystem::GlobalNonPagedAllocator(), L"ABC"));

    Key = 55;
    Result =  Tbl.Put(Key, KWString(KtlSystem::GlobalNonPagedAllocator(),L"DEF"));

    ULONG Count = Tbl.Count();

    if (Count != 2 || Tbl.Size() != 121)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ULONG Saturation = Tbl.Saturation();
    Saturation;

    KWString Str(KtlSystem::GlobalNonPagedAllocator());

    Result = Tbl.Get(Key, Str);

    if (!NT_SUCCESS(Result))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KWString Cmp(KtlSystem::GlobalNonPagedAllocator(),L"DEF");

    if (Str != Cmp)
    {
        return STATUS_UNSUCCESSFUL;
    }



    // Iterate

    Tbl.Reset();


    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (1)
    {
        ULONG K;
        KWString D(KtlSystem::GlobalNonPagedAllocator());

        Result = Tbl.Next(K, D);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        //KTestPrintf("Key=%u String=%S\n", K, PWSTR(D));
    }


    Tbl.Remove(Key);

    Result = Tbl.Get(Key, Str);

    if (Result != STATUS_NOT_FOUND)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Tbl.Clear();

    if (Tbl.Count() != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}

// More extensive
//
// Table to store 10,000 GUIDs and their string equivalents
//
//
NTSTATUS
Test2()
{
    const ULONG TEST_LIMIT = 5077;

    KHashTable<GUID, KWString> Table(TEST_LIMIT, K_DefaultHashFunction, *gp_Allocator);
    KArray<GUID> List(*gp_Allocator);

    for (ULONG i = 0; i < TEST_LIMIT; i++)
    {
        KGuid g;
        g.CreateNew();

        KWString GuidStr(*gp_Allocator, g);
        Table.Put(g, GuidStr);

        ULONG Count = Table.Count();
        ULONG Saturation = Table.Saturation();
        Count;
        Saturation;

       // KTestPrintf("Adding %S [Count=%u Saturation=%u \n", PWSTR(GuidStr), Count, Saturation);
        List.Append(g);
    }

    ULONG Worst = 0;
    ULONG S1, S2, S3;
    ULONG TmpWorst;

    for (ULONG i = 0; i < TEST_LIMIT; i++)
    {
        GUID g = List[i];

        KWString Str(*gp_Allocator);
        Table.Get(g, Str);

        KWString Test(*gp_Allocator, g);
        if (Test != Str)
        {
            return STATUS_UNSUCCESSFUL;
        }

        Table.GetStatistics(S1, S2, S3, TmpWorst);

        if (TmpWorst > Worst)
        {
            Worst = TmpWorst;
            KTestPrintf("Test 2 WorstSearch=%u\n", TmpWorst);
        }
    }

    KTestPrintf("Size = %u, Saturation=%u, Search1=%u Search2=%u Search3=%u\n", Table.Count(), Table.Saturation(), S1, S2, S3);

    return STATUS_SUCCESS;
}

inline ULONG SpecialHash(const KWString& Src)
{
    return KChecksum::Crc32(PWSTR(Src), Src.Length()/4, 0);
}


NTSTATUS
Test3()
{
    const ULONG TEST_LIMIT = 32767;

    KHashTable<KWString, GUID> Table(TEST_LIMIT, SpecialHash, *gp_Allocator);
    KArray<GUID> List(*gp_Allocator);

    for (ULONG i = 0; i < TEST_LIMIT; i++)
    {
        KGuid g;
        g.CreateNew();

        KWString GuidStr(*gp_Allocator, g);
        Table.Put(GuidStr, g);

        ULONG Count = Table.Count();
        ULONG Saturation = Table.Saturation();
        Count;
        Saturation;

       // KTestPrintf("Adding %S [Count=%u Saturation=%u \n", PWSTR(GuidStr), Count, Saturation);
        List.Append(g);
    }

    KInvariant(Table.Saturation() == 100);

    ULONG Worst = 0;
    ULONG S1, S2, S3;
    ULONG TmpWorst;

    for (ULONG i = 0; i < TEST_LIMIT; i++)
    {
        GUID g = List[i];

        KWString KeyStr(*gp_Allocator, g);
        GUID TestGuid;

        Table.Get(KeyStr, TestGuid);

        if (g != TestGuid)
        {
            return STATUS_UNSUCCESSFUL;
        }

        Table.GetStatistics(S1, S2, S3, TmpWorst);

        if (TmpWorst > Worst)
        {
            Worst = TmpWorst;
            KTestPrintf("Test 2 WorstSearch=%u\n", TmpWorst);
        }
    }

    KTestPrintf("Size = %u, Saturation=%u, Search1=%u Search2=%u Search3=%u\n", Table.Count(), Table.Saturation(), S1, S2, S3);

    for (ULONG i = 0; i < TEST_LIMIT; i++)
    {
        GUID g = List[i];

        KWString KeyStr(*gp_Allocator, g);

        NTSTATUS Result = Table.Remove(KeyStr);

        if (Result == STATUS_NOT_FOUND)
        {
            KTestPrintf("Could not find key %S at %u\n", PWSTR(KeyStr), i);
        }
    }

    ULONG Count = Table.Count();
    KTestPrintf("Final count = %u\n", Count);

    if (Count)
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}

NTSTATUS
Test4()
{
    KHashTable<ULONG, ULONG>* p = _new(KTL_TAG_TEST, *gp_Allocator) KHashTable<ULONG, ULONG>(121, K_DefaultHashFunction, *gp_Allocator);
    KFinally([&](){ _delete(p); });

    KHashTable<ULONG, ULONG*> Test2(121, K_DefaultHashFunction, *gp_Allocator);

    ULONG k = 2, v = 33;
    p->Put(k, v);

    PULONG Pu = &v;
    Test2.Put(k, Pu);

    return STATUS_SUCCESS;
}

struct MyStruct
{
    ULONG Garbage;
    ULONG Key;
    ULONG Value;
    KListEntry Link;

    MyStruct(ULONG k, ULONG v)
    {
        Garbage = 7;
        Key = k;
        Value = v;
    }
};

NTSTATUS
Test10()
{
    NTSTATUS Result;

    KNodeHashTable<ULONG, MyStruct> Tbl(
        121,
        K_DefaultHashFunction,
        FIELD_OFFSET(MyStruct, Key),
        FIELD_OFFSET(MyStruct, Link),
        *gp_Allocator
        );

    MyStruct s1(101, 201);
    MyStruct s2(102, 202);

    Tbl.Put(&s1);
    Tbl.Put(&s2);


    MyStruct* Target = nullptr;

    ULONG Key = 101;
    Tbl.Get(Key , Target);

    if (Target->Key != Key || Target->Value != 201)
    {
        Tbl.Clear();
        return STATUS_UNSUCCESSFUL;
    }


    Key = 102;
    Tbl.Get(Key, Target);

    if (Target->Key != Key || Target->Value != 202)
    {
        Tbl.Clear();
        return STATUS_UNSUCCESSFUL;
    }

    s1.Value = 999;
    Tbl.Put(&s1);

    Target = nullptr;

    Tbl.Get(Key, Target);

    if (Target->Key != Key || Target->Value != 202)
    {
        Tbl.Clear();
        return STATUS_UNSUCCESSFUL;
    }

    Key = 101;
    Tbl.Get(Key, Target);
    if (Target->Key != 101 || Target->Value != 999)
    {
        Tbl.Clear();
        return STATUS_UNSUCCESSFUL;
    }

    Tbl.Remove(Target);
    Target = nullptr;

    Result = Tbl.Get(Key, Target);

    if (Result != STATUS_NOT_FOUND)
    {
        Tbl.Clear();
        return STATUS_UNSUCCESSFUL;
    }

    Tbl.Clear();
    return STATUS_SUCCESS;
}

void
ResizeTest()
{
    KAllocator&                 allocator = KtlSystem::GlobalNonPagedAllocator();

    #define KTL_HASH_STATS
    KHashTable<GUID, ULONG>    ht1(1, K_DefaultHashFunction, allocator);
    #undef KTL_HASH_STATS

    NTSTATUS                    status = ht1.Status();

    //* Prove basic Resize() behavior
    KInvariant(NT_SUCCESS(status));
    KInvariant(ht1.Size() == 1);
    KInvariant(ht1.Count() == 0);

    status = ht1.Resize(2);
    KInvariant(NT_SUCCESS(status));
    KInvariant(ht1.Size() == 2);
    KInvariant(ht1.Count() == 0);

    status = ht1.Resize(1);
    KInvariant(NT_SUCCESS(status));
    KInvariant(ht1.Size() == 1);
    KInvariant(ht1.Count() == 0);

    KInvariant(ht1.Saturation() == 0);

    ULONG       targetSize = 10000;
    {
        for (ULONG ix = 0; ix < targetSize; ix++)
        {
            KGuid   key;
            ULONG   prevValue = MAXULONG;

            key.CreateNew();

            status = ht1.Put(key, ix, FALSE, &prevValue);
            KInvariant(NT_SUCCESS(status));
        }
    }

    KInvariant(ht1.Size() == 1);
    KInvariant(ht1.Count() == targetSize);
    ULONG   saturation = ht1.Saturation();
    KInvariant(saturation == (targetSize * 100));

    for (ULONG newSize = ht1.Size() * 2; newSize < (targetSize * 20); newSize += (newSize >> 1))
    {
        status = ht1.Resize(newSize);
        KInvariant(NT_SUCCESS(status));
        KInvariant(ht1.Size() == newSize);
        KInvariant(ht1.Count() == targetSize);

        saturation = ht1.Saturation();
        KInvariant(saturation == (targetSize * 100) / newSize);

        KTestPrintf("ResizeTest: Size: %u; Sat: %u%%  -- ", ht1.Size(), saturation);

        GUID    key;
        ULONG   value;
        while (ht1.Next(key, value) == STATUS_SUCCESS)
        {
            KInvariant(ht1.ContainsKey(key));
        }

        ULONG       search1;
        ULONG       search2;
        ULONG       search3;
        ULONG       searchWorst;
        ht1.GetStatistics(search1, search2, search3, searchWorst);

        KTestPrintf(
            "\tSearch1: %u; Search2: %u; Search=>3: %u; WorstDepth: %u\n",
            search1,
            search2,
            search3,
            searchWorst);
    }

    #define KTL_HASH_STATS
    KHashTable<ULONGLONG, ULONG>    ht2(1, K_DefaultHashFunction, allocator);
    #undef KTL_HASH_STATS

    status = ht2.Status();
    KInvariant(NT_SUCCESS(status));

    {
        for (ULONG ix = 0; ix < targetSize; ix++)
        {
            ULONG       prevValue = MAXULONG;
            ULONGLONG   key = ix;
            status = ht2.Put(key, ix, FALSE, &prevValue);
            KInvariant(NT_SUCCESS(status));
        }
    }

    KInvariant(ht2.Size() == 1);
    KInvariant(ht2.Count() == targetSize);
    saturation = ht2.Saturation();
    KInvariant(saturation == (targetSize * 100));

    for (ULONG newSize = ht2.Size() * 2; newSize < (targetSize * 20); newSize += (newSize >> 1))
    {
        status = ht2.Resize(newSize);
        KInvariant(NT_SUCCESS(status));
        KInvariant(ht2.Size() == newSize);
        KInvariant(ht2.Count() == targetSize);

        saturation = ht2.Saturation();
        KInvariant(saturation == (targetSize * 100) / newSize);

        KTestPrintf("ResizeTest: Size: %u; Sat: %u%%  -- ", ht2.Size(), saturation);

        ULONGLONG   key;
        ULONG       value;
        while (ht2.Next(key, value) == STATUS_SUCCESS)
        {
            KInvariant(ht2.ContainsKey(key));
        }

        ULONG       search1;
        ULONG       search2;
        ULONG       search3;
        ULONG       searchWorst;
        ht2.GetStatistics(search1, search2, search3, searchWorst);

        KTestPrintf(
            "\tSearch1: %u; Search2: %u; Search=>3: %u; WorstDepth: %u\n",
            search1,
            search2,
            search3,
            searchWorst);
    }
}


void
NodeResizeTest()
{
    KAllocator&                 allocator = KtlSystem::GlobalNonPagedAllocator();

    struct TestStruct
    {
        ULONG           ItemNumber;
        GUID            Key;
        KListEntry      HTList;
    };

    #define KTL_HASH_STATS
    KNodeHashTable<GUID, TestStruct> ht1(
        1,
        K_DefaultHashFunction,
        FIELD_OFFSET(TestStruct, Key),
        FIELD_OFFSET(TestStruct, HTList),
        allocator);
    #undef KTL_HASH_STATS

    NTSTATUS                    status = ht1.Status();

    //* Prove basic Resize() behavior
    KInvariant(NT_SUCCESS(status));
    KInvariant(ht1.Size() == 1);
    KInvariant(ht1.Count() == 0);

    status = ht1.Resize(2);
    KInvariant(NT_SUCCESS(status));
    KInvariant(ht1.Size() == 2);
    KInvariant(ht1.Count() == 0);

    status = ht1.Resize(1);
    KInvariant(NT_SUCCESS(status));
    KInvariant(ht1.Size() == 1);
    KInvariant(ht1.Count() == 0);

    KInvariant(ht1.Saturation() == 0);

    ULONG       targetSize = 10000;
    {
        for (ULONG ix = 0; ix < targetSize; ix++)
        {
            KGuid           key;
            TestStruct*     prevValue = nullptr;

            key.CreateNew();

            KUniquePtr<TestStruct>  item = _new(KTL_TAG_TEST, allocator) TestStruct();
            KInvariant(item);

            item->ItemNumber = ix;
            item->Key = key;

            status = ht1.Put(item.Detach(), FALSE, &prevValue);
            KInvariant(NT_SUCCESS(status));
            KInvariant(prevValue == nullptr);
        }
    }

    KFinally
    (
        [&]() -> void
        {
            ht1.Reset();
            TestStruct*     currItem = nullptr;

            while ((status = ht1.Next(currItem)) != STATUS_NO_MORE_ENTRIES)
            {
                KInvariant(NT_SUCCESS(status));
                KInvariant(NT_SUCCESS(ht1.Remove(currItem)));
                _delete(currItem);
            }
        }
    );

    KInvariant(ht1.Size() == 1);
    KInvariant(ht1.Count() == targetSize);
    ULONG   saturation = ht1.Saturation();
    KInvariant(saturation == (targetSize * 100));

    for (ULONG newSize = ht1.Size() * 2; newSize < (targetSize * 20); newSize += (newSize >> 1))
    {
        status = ht1.Resize(newSize);
        KInvariant(NT_SUCCESS(status));
        KInvariant(ht1.Size() == newSize);
        KInvariant(ht1.Count() == targetSize);

        saturation = ht1.Saturation();
        KInvariant(saturation == (targetSize * 100) / newSize);

        KTestPrintf("ResizeTest: Size: %u; Sat: %u%%  -- ", ht1.Size(), saturation);

        TestStruct*     currItem;
        while (ht1.Next(currItem) == STATUS_SUCCESS)
        {
            KInvariant(ht1.ContainsKey(currItem->Key));
        }

        ULONG       search1;
        ULONG       search2;
        ULONG       search3;
        ULONG       searchWorst;
        ht1.GetStatistics(search1, search2, search3, searchWorst);

        KTestPrintf(
            "\tSearch1: %u; Search2: %u; Search=>3: %u; WorstDepth: %u\n",
            search1,
            search2,
            search3,
            searchWorst);
    }

    #define KTL_HASH_STATS
    KNodeHashTable<GUID, TestStruct> ht2(
        1,
        K_DefaultHashFunction,
        FIELD_OFFSET(TestStruct, Key),
        FIELD_OFFSET(TestStruct, HTList),
        allocator);
    #undef KTL_HASH_STATS

    {
        for (ULONG ix = 0; ix < targetSize; ix++)
        {
            KGuid           key;
            TestStruct*     prevValue = nullptr;

            key.CreateNew();

            KUniquePtr<TestStruct>  item = _new(KTL_TAG_TEST, allocator) TestStruct();
            KInvariant(item);

            item->ItemNumber = ix;
            item->Key = key;

            status = ht2.Put(item.Detach(), FALSE, &prevValue);
            KInvariant(NT_SUCCESS(status));
            KInvariant(prevValue == nullptr);
        }
    }

    KInvariant(ht2.Size() == 1);
    KInvariant(ht2.Count() == targetSize);
    saturation = ht2.Saturation();
    KInvariant(saturation == (targetSize * 100));

    KFinally
    (
        [&]() -> void
        {
            ht2.Reset();
            TestStruct*     currItem = nullptr;

            while ((status = ht2.Next(currItem)) != STATUS_NO_MORE_ENTRIES)
            {
                KInvariant(NT_SUCCESS(status));
                KInvariant(NT_SUCCESS(ht2.Remove(currItem)));
                _delete(currItem);
            }
        }
    );

    for (ULONG newSize = ht2.Size() * 2; newSize < (targetSize * 20); newSize += (newSize >> 1))
    {
        status = ht2.Resize(newSize);
        KInvariant(NT_SUCCESS(status));
        KInvariant(ht2.Size() == newSize);
        KInvariant(ht2.Count() == targetSize);

        saturation = ht2.Saturation();
        KInvariant(saturation == (targetSize * 100) / newSize);

        KTestPrintf("ResizeTest: Size: %u; Sat: %u%%  -- ", ht2.Size(), saturation);

        TestStruct* item = nullptr;
        while (ht2.Next(item) == STATUS_SUCCESS)
        {
            KInvariant(ht2.ContainsKey(item->Key));
        }

        ULONG       search1;
        ULONG       search2;
        ULONG       search3;
        ULONG       searchWorst;
        ht2.GetStatistics(search1, search2, search3, searchWorst);

        KTestPrintf(
            "\tSearch1: %u; Search2: %u; Search=>3: %u; WorstDepth: %u\n",
            search1,
            search2,
            search3,
            searchWorst);
    }
}

VOID
Scenario_KUriView_Test()
{
    NTSTATUS Result;

    // Inputs
    KUriView uriA1(L"fabric:/a");
    KUriView uriA2(L"fabric:/a");
    KUriView uriB(L"fabric:/b");

    // Setup
    KHashTable<KUriView, KUriView> hashTable(17, K_DefaultHashFunction, *gp_Allocator);

    Result = hashTable.Put(uriA1, uriA1, FALSE);
    KInvariant(NT_SUCCESS(Result));

    BOOLEAN isSuccess = hashTable.ContainsKey(uriA1);
    KInvariant(isSuccess == TRUE);

    isSuccess = hashTable.ContainsKey(uriA2);
    KInvariant(isSuccess == TRUE);

    Result = hashTable.Put(uriA1, uriB, TRUE);
    KInvariant(NT_SUCCESS(Result));

    KUriView uriResult;
    Result = hashTable.Get(uriA1, uriResult);
    KInvariant(NT_SUCCESS(Result));
    KInvariant(uriB == uriResult);
}

BOOLEAN EqualityComparer(KUri::SPtr const & itemOne, KUri::SPtr const & itemTwo)
{
    return *itemOne == *itemTwo;
}

BOOLEAN EqualityComparer(KUri::CSPtr const & itemOne, KUri::CSPtr const & itemTwo)
{
    return *itemOne == *itemTwo;
}

VOID
Scenario_KUriSPtr_Test()
{
    NTSTATUS Result;

    // Inputs
    KUriView uriA1View(L"fabric:/a");
    KUriView uriA2View(L"fabric:/a");
    KUriView uriBView(L"fabric:/b");

    KUri::SPtr uriA1 = nullptr;
    Result = KUri::Create(uriA1View, *gp_Allocator, uriA1);
    KInvariant(NT_SUCCESS(Result));

    KUri::SPtr uriA2 = nullptr;
    Result = KUri::Create(uriA1View, *gp_Allocator, uriA2);
    KInvariant(NT_SUCCESS(Result));

    KUri::SPtr uriB = nullptr;
    Result = KUri::Create(uriBView, *gp_Allocator, uriB);
    KInvariant(NT_SUCCESS(Result));

    // Setup
    KHashTable<KUri::SPtr, KUri::SPtr> hashTable(*gp_Allocator);
    hashTable.Initialize(8, K_DefaultHashFunction, EqualityComparer);
    KInvariant(NT_SUCCESS(hashTable.Status()));

    // Test
    Result = hashTable.Put(uriA1, uriA1, FALSE);
    KInvariant(NT_SUCCESS(Result));

    BOOLEAN isSuccess = hashTable.ContainsKey(uriA1);
    KInvariant(isSuccess == TRUE);

    isSuccess = hashTable.ContainsKey(uriA2);
    KInvariant(isSuccess == TRUE);

    Result = hashTable.Put(uriA1, uriB, TRUE);
    KInvariant(NT_SUCCESS(Result));

    KUri::SPtr uriResult;
    Result = hashTable.Get(uriA1, uriResult);
    KInvariant(NT_SUCCESS(Result));
    KInvariant(*uriB == *uriResult);
}

VOID
Scenario_KUriCSPtr_Test()
{
    NTSTATUS Result;

    // Inputs
    KUriView uriA1View(L"fabric:/a");
    KUriView uriA2View(L"fabric:/a");
    KUriView uriBView(L"fabric:/b");

    KUri::CSPtr uriA1 = nullptr;
    KUri::CSPtr uriA2 = nullptr;
    KUri::CSPtr uriB = nullptr;

    Result = KUri::Create(uriA1View, *gp_Allocator, uriA1);
    KInvariant(NT_SUCCESS(Result));

    Result = KUri::Create(uriA1View, *gp_Allocator, uriA2);
    KInvariant(NT_SUCCESS(Result));

    Result = KUri::Create(uriBView, *gp_Allocator, uriB);
    KInvariant(NT_SUCCESS(Result));

    // Setup
    KHashTable<KUri::CSPtr, KUri::CSPtr> hashTable(17, K_DefaultHashFunction, EqualityComparer, *gp_Allocator);
    KInvariant(NT_SUCCESS(hashTable.Status()));

    // Test
    Result = hashTable.Put(uriA1, uriA1, FALSE);
    KInvariant(NT_SUCCESS(Result));

    BOOLEAN isSuccess = hashTable.ContainsKey(uriA1);
    KInvariant(isSuccess == TRUE);

    isSuccess = hashTable.ContainsKey(uriA2);
    KInvariant(isSuccess == TRUE);

    Result = hashTable.Put(uriA1, uriB, TRUE);
    KInvariant(NT_SUCCESS(Result));

    KUri::CSPtr uriResult;
    Result = hashTable.Get(uriA1, uriResult);
    KInvariant(NT_SUCCESS(Result));
    KInvariant(*uriB == *uriResult);
}

VOID
Scenario_KUriCSPtr_MovedHashTable_Test()
{
    NTSTATUS Result;

    // Inputs
    KUriView uriA1View(L"fabric:/a");
    KUriView uriA2View(L"fabric:/a");
    KUriView uriBView(L"fabric:/b");

    KUri::CSPtr uriA1 = nullptr;
    Result = KUri::Create(uriA1View, *gp_Allocator, uriA1);
    KInvariant(NT_SUCCESS(Result));

    KUri::CSPtr uriA2 = nullptr;
    Result = KUri::Create(uriA1View, *gp_Allocator, uriA2);
    KInvariant(NT_SUCCESS(Result));

    KUri::CSPtr uriB = nullptr;
    Result = KUri::Create(uriBView, *gp_Allocator, uriB);
    KInvariant(NT_SUCCESS(Result));

    // Setup
    KHashTable<KUri::CSPtr, KUri::CSPtr> tmpHashTable(17, K_DefaultHashFunction, EqualityComparer, *gp_Allocator);
    KInvariant(NT_SUCCESS(tmpHashTable.Status()));

    KHashTable<KUri::CSPtr, KUri::CSPtr> hashTable(Ktl::Move(tmpHashTable));
    KInvariant(NT_SUCCESS(hashTable.Status()));

    // Test
    Result = hashTable.Put(uriA1, uriA1, FALSE);
    KInvariant(NT_SUCCESS(Result));

    BOOLEAN isSuccess = hashTable.ContainsKey(uriA1);
    KInvariant(isSuccess == TRUE);

    isSuccess = hashTable.ContainsKey(uriA2);
    KInvariant(isSuccess == TRUE);

    Result = hashTable.Put(uriA1, uriB, TRUE);
    KInvariant(NT_SUCCESS(Result));

    KUri::CSPtr uriResult;
    Result = hashTable.Get(uriA1, uriResult);
    KInvariant(NT_SUCCESS(Result));
    KInvariant(*uriB == *uriResult);
}


void
MoveTest()
{
    KHashTable<ULONG, KWString> Tbl(5, MyHashFunction, *gp_Allocator);

    NTSTATUS Result;

    ULONG Key = 32;

    Result =  Tbl.Put(Key, KWString(KtlSystem::GlobalNonPagedAllocator(), L"ABC"));
    KInvariant(Result == S_OK);

    Key = 55;
    Result =  Tbl.Put(Key, KWString(KtlSystem::GlobalNonPagedAllocator(),L"DEF"));
    KInvariant(Result == S_OK);

    Key = 1;
    Result =  Tbl.Put(Key, KWString(KtlSystem::GlobalNonPagedAllocator(),L"GH"));
    KInvariant(Result == S_OK);

    ULONG getKey;
    KWString getValue(KtlSystem::GlobalNonPagedAllocator(), L"temp");
    Tbl.Reset();

    Result = Tbl.Next(getKey, getValue);
    KInvariant(Result == S_OK);

    // Part 1 : Move constructor test
    KHashTable<ULONG, KWString> MovedTbl(Ktl::Move(Tbl));

    // Verify old table content
    // Old table must be empty
    KInvariant(Tbl.Count() == 0);
    KInvariant(Tbl.Size() == 0);
    // Any existing Old table enumerators must return no_more_entries
    Result = Tbl.Next(getKey, getValue);
    KInvariant(Result == STATUS_NO_MORE_ENTRIES);

    // Verify new table content
    KInvariant(MovedTbl.Count() == 3);
    KInvariant(MovedTbl.Size() == 5);
    // Any existing new table enumerators must return the next item if available
    Result = MovedTbl.Next(getKey, getValue);
    KInvariant(Result == S_OK);

    // Part 2 : Move assignment operator test
    Key = 999;
    KHashTable<ULONG, KWString> Tbl2(7, MyHashFunction, *gp_Allocator);
    Result =  Tbl2.Put(Key, KWString(KtlSystem::GlobalNonPagedAllocator(), L"XYZ"));

    MovedTbl = Ktl::Move(Tbl2);

    // Verify old table content

    // Old table must be empty
    KInvariant(Tbl2.Count() == 0);
    KInvariant(Tbl2.Size() == 0);

    // Verify new table content
    KInvariant(MovedTbl.Count() == 1);
    KInvariant(MovedTbl.Size() == 7);
}

void
Hash_LongLong_Zero()
{
    ULONGLONG inputULongLong = 0;
    LONGLONG inputLongLong = 0;
    KInvariant(K_DefaultHashFunction(inputULongLong) == K_DefaultHashFunction(inputLongLong));
}

void
Hash_LongLong_Positive()
{
    ULONGLONG inputULongLong = 1024;
    LONGLONG inputLongLong = 1024;
    KInvariant(K_DefaultHashFunction(inputULongLong) == K_DefaultHashFunction(inputLongLong));
}

void
Hash_LongLong_Negative()
{
    LONGLONG inputLongLong = -1024;
    ULONGLONG inputULongLong = ~inputLongLong;

    KInvariant(K_DefaultHashFunction(inputULongLong) == K_DefaultHashFunction(inputLongLong));
}

void
Hash_KStringView_Deterministic()
{
    LPCWSTR stateProviderName = L"fabric:/stateprovides/stateprovider";

    KStringView stringView1(stateProviderName);
    KStringView stringView2(stateProviderName);

    ULONG hashOne = K_DefaultHashFunction(stringView1);
    ULONG hashTwo = K_DefaultHashFunction(stringView2);

    KInvariant(hashOne == hashTwo);
}

void
Hash_KStringSmartPointer_Deterministic()
{
    NTSTATUS status;
    LPCWSTR stateProviderName = L"fabric:/stateprovides/stateprovider";

    KStringView stringView(stateProviderName);
    KString::SPtr stringSPtr = nullptr;
    status = KString::Create(stringSPtr, *gp_Allocator, stateProviderName);
    KInvariant(status == STATUS_SUCCESS);

    KString::SPtr stringSPtrTmp = nullptr;
    status = KString::Create(stringSPtrTmp, *gp_Allocator, stateProviderName);
    KInvariant(status == STATUS_SUCCESS);

    KString::CSPtr stringCSPtr = (KString::CSPtr&)stringSPtrTmp;

    ULONG hashOne = K_DefaultHashFunction(stringView);
    ULONG hashTwo = K_DefaultHashFunction(stringSPtr);
    ULONG hashThree = K_DefaultHashFunction(stringCSPtr);

    KInvariant(hashOne == hashTwo);
    KInvariant(hashOne == hashThree);
}

void
Hash_KUriView_Deterministic()
{
    LPCWSTR stateProviderName = L"fabric:/stateprovides/stateprovider";

    KUriView uriView1(stateProviderName);
    KUriView uriView2(stateProviderName);

    ULONG hashOne = K_DefaultHashFunction(uriView1);
    ULONG hashTwo = K_DefaultHashFunction(uriView2);

    KInvariant(hashOne == hashTwo);
}

void
Hash_KUriSmartPointer_Deterministic()
{
    NTSTATUS status;
    LPCWSTR stateProviderName = L"fabric:/stateprovides/stateprovider";

    const KUriView constUriView(stateProviderName);

    KUri::SPtr uriSPtr = nullptr;
    status = KUri::Create(KUriView(stateProviderName), *gp_Allocator, uriSPtr);
    KInvariant(status == STATUS_SUCCESS);

    KUri::SPtr uriSPtrTmp = nullptr;
    status = KUri::Create(KUriView(stateProviderName), *gp_Allocator, uriSPtrTmp);
    KInvariant(status == STATUS_SUCCESS);

    KUri::CSPtr uriCSPtr = (KUri::CSPtr&)uriSPtrTmp;

    ULONG hashOne = K_DefaultHashFunction(constUriView);
    ULONG hashTwo = K_DefaultHashFunction(uriSPtr);
    ULONG hashThree = K_DefaultHashFunction(uriCSPtr);

    KInvariant(hashOne == hashTwo);
    KInvariant(hashOne == hashThree);
}

void SaturationTest()
{
    KHashTable<ULONG, ULONG> table(100, MyHashFunction, *gp_Allocator);

    for (ULONG i = 0; i < 50; i++)
    {
        KInvariant(table.Saturation() == i);        // S/B i% sat
        KInvariant(NT_SUCCESS(table.Put(i, i)));
    }

    KInvariant(table.Saturation() == 50);       // S/B 50% sat
    for (ULONG x = 50; x < 100; x++) KInvariant(NT_SUCCESS(table.Put(x, x)));
    KInvariant(table.Saturation() == 100);      // S/B 100% sat
    for (ULONG x = 100; x < 200; x++) KInvariant(NT_SUCCESS(table.Put(x, x)));
    KInvariant(table.Saturation() == 200);      // S/B 200% sat

    KInvariant(NT_SUCCESS(table.Resize(2 * 200)));
    KInvariant(table.Saturation() == 50);       // S/B 50% sat
    KInvariant(table.Saturation(100) == 200);   // S/B 200% sat
    KInvariant(table.Saturation(800) == 25);    // S/B 25% sat
    KInvariant(table.Saturation(20000) == 1);   // S/B 1% sat
    KInvariant(NT_SUCCESS(table.Resize(20000)));
    KInvariant(table.Saturation() == 1);        // S/B 1% sat
    KInvariant(NT_SUCCESS(table.Resize(1)));
    KInvariant(table.Saturation() == 20000);    // S/B 20000% sat
    table.Clear();
    KInvariant(table.Saturation() == 0);        // S/B 0% sat
}

NTSTATUS
TestSequence()
{
    NTSTATUS Result = STATUS_SUCCESS;
    unsigned Iterations = 1;

#if KTL_USER_MODE
    DWORD dwStart = GetTickCount();
#endif

    SaturationTest();
    
    for (ULONG i = 0; i < Iterations; i++)
    {
        Result = Test1();

        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        Result = Test2();
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        Result = Test3();
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        Result = Test10();
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        ResizeTest();
        NodeResizeTest();
        MoveTest();

        Scenario_KUriView_Test();
        Scenario_KUriSPtr_Test();
        Scenario_KUriCSPtr_Test();
        Scenario_KUriCSPtr_MovedHashTable_Test();

        // Hash tests.
        Hash_LongLong_Zero();
        Hash_LongLong_Positive();
        Hash_LongLong_Negative();

        Hash_KStringView_Deterministic();
        Hash_KUriView_Deterministic();

        Hash_KStringSmartPointer_Deterministic();
        Hash_KUriSmartPointer_Deterministic();
    }

#if KTL_USER_MODE
    DWORD dwStop = GetTickCount();
    KTestPrintf("Elapsed = %d\n", dwStop - dwStart);
#endif

    return Result;
}

//* A simple extension to KHashTable for the Native data stack runtime - as a default
//  adhoc HashTable based on doubling and halfing of hash buckets rounded to the nearest
//  prime number up to approx 3G buckets
//
template <class KeyType, class DataType>
class KAutoHashTable : public KHashTable<KeyType, DataType>
{
    K_DENY_COPY(KAutoHashTable);

private:
    __forceinline void ResizeUpIf()
    {
        ULONG savedIx = _currentPrimeIx;

        while ((this->Saturation(_gDoublePrimes[_currentPrimeIx]) > _autoUpsizeThreashold) &&
               (_currentPrimeIx < (_gDoublePrimesSize - 1)))               // Increase if above 60%
        {
            // Resize up:
            _currentPrimeIx++;
        }

        if (savedIx != _currentPrimeIx)
        {
            if (!NT_SUCCESS(__super::Resize(_gDoublePrimes[_currentPrimeIx])))
            {
                _currentPrimeIx = savedIx;
            }
        }
    }

    __forceinline void ResizeDownIf()
    {
        ULONG savedIx = _currentPrimeIx;

        while ((this->Saturation(_gDoublePrimes[_currentPrimeIx]) < _autoDownsizeThreashold) &&
               (_currentPrimeIx > 0))                                       // Decrease if below 30%
        {
            // Resize down using the prev in sequence
            _currentPrimeIx--;
        }

        if (savedIx != _currentPrimeIx)
        {
            if (!NT_SUCCESS(__super::Resize(_gDoublePrimes[_currentPrimeIx])))
            {
                _currentPrimeIx = savedIx;
            }
        }
    }

public:
    static const ULONG          DefaultAutoUpsizeThresholdPrectage = 60;
    static const ULONG          DefaultAutoDownsizeThresholdPrectage = 30;

    FAILABLE KAutoHashTable(
        __in ULONG Size,
        __in typename KHashTable<KeyType, DataType>::HashFunctionType Func,
        __in KAllocator& Alloc)
        :  KHashTable<KeyType, DataType>(Alloc)
    {
        if (NT_SUCCESS(this->Status()))
        {
            _autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
            _autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
            _autoSizeIsEnabled = true;
            _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
            Size = _gDoublePrimes[_currentPrimeIx];
            this->SetConstructorStatus(__super::Initialize(Size, Func));
        }
    }

    FAILABLE KAutoHashTable(
        __in ULONG Size,
        __in typename KHashTable<KeyType, DataType>::HashFunctionType Func,
        __in typename KHashTable<KeyType, DataType>::EqualityComparisonFunctionType EqualityComparisonFunctionType,
        __in KAllocator& Alloc)
        : KHashTable<KeyType, DataType>(Alloc)
    {
        if (NT_SUCCESS(this->Status()))
        {
            _autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
            _autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
            _autoSizeIsEnabled = true;
            _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
            Size = _gDoublePrimes[_currentPrimeIx];
            this->SetConstructorStatus(__super::Initialize(Size, Func, EqualityComparisonFunctionType));
        }
    }

    // Default constructor
    //
    // This allows the hash table to be embedded in another object and be initialized during later execution once
    // the values for Initialize() are known.
    //
    NOFAIL KAutoHashTable(__in KAllocator& Alloc)
        : KHashTable<KeyType, DataType>(Alloc)
    {
        _autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
        _autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
        _autoSizeIsEnabled = true;
        _currentPrimeIx = 0;
    }

    // Move constructor.
    NOFAIL KAutoHashTable(__in KAutoHashTable&& Src)
        : KHashTable<KeyType, DataType>(Ktl::Move(*((KHashTable<KeyType, DataType>*)&Src)))
    {
        _autoUpsizeThreashold = Src._autoUpsizeThreashold;
        _autoDownsizeThreashold = Src._autoDownsizeThreashold;
        _autoSizeIsEnabled = Src._autoSizeIsEnabled;
        _currentPrimeIx = Src._currentPrimeIx;
        Src._currentPrimeIx = 0;
        Src._autoSizeIsEnabled = true;
        Src._autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
        Src._autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
    }

    // Move assignment operator.
    NOFAIL KAutoHashTable& operator=(__in KAutoHashTable&& Src)
    {
        if (&Src == this)
        {
            return *this;
        }

        __super::operator=(Ktl::Move(Src));
        _autoUpsizeThreashold = Src._autoUpsizeThreashold;
        _autoDownsizeThreashold = Src._autoDownsizeThreashold;
        _autoSizeIsEnabled = Src._autoSizeIsEnabled;
        _currentPrimeIx = Src._currentPrimeIx;
        Src._currentPrimeIx = 0;
        Src._autoSizeIsEnabled = true;
        Src._autoUpsizeThreashold = DefaultAutoUpsizeThresholdPrectage;
        Src._autoDownsizeThreashold = DefaultAutoDownsizeThresholdPrectage;
        return *this;
    }

    //* Post CTOR Initializers
    NTSTATUS Initialize(__in ULONG Size, __in typename KHashTable<KeyType, DataType>::HashFunctionType Func)
    {
        _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
        Size = _gKDoublePrimes[_currentPrimeIx];
        return __super::Initialize(Size, Func);
    }

    NTSTATUS Initialize(
        __in ULONG Size, 
        __in typename KHashTable<KeyType, DataType>::HashFunctionType Func,
        __in typename KHashTable<KeyType, DataType>::EqualityComparisonFunctionType EqualityComparitionFunc)
    {
        _currentPrimeIx = ClosestNextPrimeNumberIx(Size);
        Size = _gKDoublePrimes[_currentPrimeIx];
        return __super::Initialize(Size, Func, EqualityComparitionFunc);
    }

    //* Disable or Enable automatic resizing - when enabled resizing will be attempted
    void SetAutoSizeEnable(__in bool ToEnabled)
    {
        if (ToEnabled && !_autoSizeIsEnabled)
        {
            ResizeUpIf();
            ResizeDownIf();
        }

        _autoSizeIsEnabled = ToEnabled;
    }

    //* Accessors for Up/Downsize thresholds
    ULONG GetUpsizeThreshold()                  { return _autoUpsizeThreashold; }
    ULONG GetDownsizeThreshold()                { return _autoDownsizeThreashold; }
    void  SetUpsizeThreshold(ULONG NewValue)    { _autoUpsizeThreashold = newValue; }
    void  SetDownsizeThreshold(ULONG NewValue)  { _autoDownsizeThreashold = newValue; }

    // Put
    //
    // Adds or updates the item.
    //
    // Parameters:
    //      Key           The key under which to store the data item
    //      Data          The data item to store.
    //      ForceUpdate   If TRUE, always write the item, even if it is an update.
    //                    If FALSE, only create, but don't update.
    //      PreviousValue Retrieves the previously existing value if one exists.
    //                    Typically used in combination with ForceUpdate==FALSE.
    //
    // Return values:
    // Return values:
    //      STATUS_SUCCESS - Item was inserted and there was no previous item with
    //          the same key.
    //      STATUS_OBJECT_NAME_COLLISION - Item was not inserted because there was an
    //          existing item with the same key and ForceUpdate was false.
    //      STATUS_OBJECT_NAME_EXISTS - Item replaced a previous item.
    //      STATUS_INSUFFICIENT_RESOURCES - Item was not inserted.
    //
    NTSTATUS Put(
        __in  const KeyType&  Key,
        __in  const DataType& Data,
        __in  BOOLEAN   ForceUpdate = TRUE,
        __out_opt DataType* PreviousValue = nullptr)
    {
        if (_autoSizeIsEnabled)
        {
            ResizeUpIf();
        }
        return __super::Put(Key, Data, ForceUpdate, PreviousValue);
    }

    // Remove
    //
    // Removes the specified Key and its associated data item from the table
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_NOT_FOUND
    //
    NTSTATUS Remove(__in const KeyType& Key, __out  DataType* Item = nullptr)
    {
        NTSTATUS result = __super::Remove(Key, Item);
        if (NT_SUCCESS(result))
        {
            if (_autoSizeIsEnabled)
            {
                ResizeDownIf();
            }
        }

        return result;
    }

    // Resize
    //
    // Resizes the hash table.
    // Parameters:
    //      NewSize         This typically should be 50% larger than the previous size,
    //                      but still a prime number.
    //
    //
    NTSTATUS Resize(__in ULONG NewSize)
    {
        _currentPrimeIx = ClosestNextPrimeNumberIx(NewSize);
        NewSize = _gDoublePrimes[_currentPrimeIx];
        return __super::Resize(NewSize);
    }

    // Clear
    //
    // Remove all entries from the table.
    //
    VOID Clear()
    {
        __super::Clear();
        _currentPrimeIx = 0;
    }

    __forceinline ULONG TestGetCurrentIx()
    {
        return _currentPrimeIx;
    }

private:
    static ULONG ClosestNextPrimeNumberIx(__in ULONG From)
    {
        ULONG   ix = 0;

        while ((ix < _gDoublePrimesSize) && (_gDoublePrimes[ix] < From)) ix++;
        return ix;
    }

private:
    ULONG       _currentPrimeIx;
    ULONG       _autoDownsizeThreashold;
    ULONG       _autoUpsizeThreashold;
    bool        _autoSizeIsEnabled;

public:
    static ULONG const      _gDoublePrimes[];
    static const ULONG      _gDoublePrimesSize;
};

//* Table of prime numbers at approx 2* intervals out to the limits of a ULONG.
//  These values are intented to used to data-drive KAutoHashTable auto upsizing 
//  and downsizing
//
template <class KeyType, class DataType>
ULONG const KAutoHashTable<KeyType, DataType>::_gDoublePrimes[] =
{
    1,
    3,
    7,
    17,
    37,
    79,
    163,
    331,
    673,
    1361,
    2729,
    5471,
    10949,
    21911,
    43853,
    87719,
    175447,
    350899,
    701819,
    1403641,
    2807303,
    5614657,
    11229331,
    22458671,
    44917381,
    89834777,
    179669557,
    359339171,
    718678369,
    1437356741,
    2874713497
};

template <class KeyType, class DataType>
const ULONG KAutoHashTable<KeyType, DataType>::_gDoublePrimesSize = sizeof(_gDoublePrimes) / sizeof(ULONG);



void TestAutoHashTable()
{
    KAutoHashTable<ULONG, ULONG>    autoHt1(1, MyHashFunction, *gp_Allocator);
    ULONG                           prevIx = autoHt1.TestGetCurrentIx();
                                    KInvariant(prevIx < (autoHt1._gDoublePrimesSize - 1));
    const ULONG                     maxItems = 10000000;
    ULONGLONG                       insertStart;
    ULONGLONG                       totalInsertsDuration;
    ULONGLONG                       removeStart;
    ULONGLONG                       totalRemovesDuration;

    KTestPrintf("\nInserting\n");
    //KInvariant(NT_SUCCESS(autoHt1.Resize(2 * maxItems)));
    //autoHt1.SetAutoSizeEnable(false);

    insertStart = KNt::GetTickCount64();
    for (ULONG i = 0; i < maxItems; i++)
    {
        /*
        KTestPrintf("TestFibonacciHashTable: Size: %u; Ix: %u; Count: %u; Sat: %u%%\n", 
            autoHt1.Size(), 
            autoHt1.TestGetCurrentIx(),
            autoHt1.Count(),
            autoHt1.Saturation());
        */

        KInvariant(NT_SUCCESS(autoHt1.Put(i, i)));
        KInvariant(prevIx <= autoHt1.TestGetCurrentIx());          // Should only go up
        prevIx = autoHt1.TestGetCurrentIx();
    }
    /*
    KTestPrintf("TestFibonacciHashTable: Size: %u; Ix: %u; Count: %u; Sat: %u%%\n",
        autoHt1.Size(),
        autoHt1.TestGetCurrentIx(),
        autoHt1.Count(),
        autoHt1.Saturation());
    */
    totalInsertsDuration = KNt::GetTickCount64() - insertStart;

    autoHt1.SetAutoSizeEnable(false);
    prevIx = autoHt1.TestGetCurrentIx();
    autoHt1.Resize(1);
    KInvariant(autoHt1.TestGetCurrentIx() == 0);
    autoHt1.SetAutoSizeEnable(true);
    KInvariant(autoHt1.TestGetCurrentIx() == prevIx);

    KTestPrintf("Removing\n");
    //autoHt1.SetAutoSizeEnable(false);

    removeStart = KNt::GetTickCount64();
    for (ULONG i = 0; i < maxItems; i++)
    {
        /*
        KTestPrintf("TestFibonacciHashTable: Size: %u; Ix: %u; Count: %u; Sat: %u%%\n",
            autoHt1.Size(),
            autoHt1.TestGetCurrentIx(),
            autoHt1.Count(),
            autoHt1.Saturation());
        */
        KInvariant(NT_SUCCESS(autoHt1.Remove(i)));
        KInvariant(prevIx >= autoHt1.TestGetCurrentIx());          // Should only go down
        prevIx = autoHt1.TestGetCurrentIx();
    }
    /*
    KTestPrintf("TestFibonacciHashTable: Size: %u; Ix: %u; Count: %u; Sat: %u%%\n",
        autoHt1.Size(),
        autoHt1.TestGetCurrentIx(),
        autoHt1.Count(),
        autoHt1.Saturation());
    */
    totalRemovesDuration = KNt::GetTickCount64() - removeStart;

    autoHt1.SetAutoSizeEnable(false);
    prevIx = autoHt1.TestGetCurrentIx();
    KInvariant(prevIx == 0);
    autoHt1.Resize(1000);
    KInvariant(autoHt1._gDoublePrimes[autoHt1.TestGetCurrentIx()] >= 1000);
    KInvariant(autoHt1._gDoublePrimes[autoHt1.TestGetCurrentIx() + 1] > (2 * 999));
    autoHt1.SetAutoSizeEnable(true);
    KInvariant(autoHt1.TestGetCurrentIx() == prevIx);

    KTestPrintf("Insert Duration: %lums; Removal Duration: %lums; for %u items\n", totalInsertsDuration, totalRemovesDuration, maxItems);

    KAutoHashTable<ULONG, ULONG>    ht2(Ktl::Move(autoHt1));
    KInvariant(ht2.Size() == 1);
    KInvariant(NT_SUCCESS(ht2.Put(1, 2)));
    KInvariant(ht2.Size() == 1);
    KInvariant(NT_SUCCESS(ht2.Put(2, 3)));
    KInvariant(ht2.Size() == 3);

    KAutoHashTable<ULONG, ULONG>    ht3(*gp_Allocator);
    KInvariant(ht3.Size() == 0);

    ht3 = Ktl::Move(ht2);
    KInvariant(ht3.Size() == 3);
    KInvariant(NT_SUCCESS(ht3.Put(3, 4)));
    KInvariant(ht3.Size() == 7);
    KInvariant(NT_SUCCESS(ht3.Put(4, 5)));
    KInvariant(ht3.Size() == 7);
}


NTSTATUS
TestBody()
{
    LONGLONG InitialAllocations;
    LONGLONG AllocationDifference;
    NTSTATUS Result = KtlSystem::Initialize();

    InitialAllocations  = KAllocatorSupport::gs_AllocsRemaining;

    gp_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    TestAutoHashTable();
    Result = TestSequence();

    AllocationDifference = KAllocatorSupport::gs_AllocsRemaining - InitialAllocations;
    KtlSystem::Shutdown();

    if (AllocationDifference != 0)
    {
        KTestPrintf("%lld Outstanding allocations\n", AllocationDifference);
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    return Result;
}

NTSTATUS
KHashTableTest(
    __in int argc, __in_ecount(argc) WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS Status;
    
#if defined(PLATFORM_UNIX)
    Status = KtlTraceRegister();
    if (! NT_SUCCESS(Status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(Status);
    }
#endif

    KTestPrintf("KHashTableTest: STARTED\n");

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KHashTableTest test");

    Status = TestBody();

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KHashTableTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif      
    
    return Status;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(__in  int argc, __in_ecount(argc) WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    return KHashTableTest(argc, args);
}
#endif
