/*++

Copyright (c) Microsoft Corporation

Module Name:

    KNodeTableTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KNodeTableTests.h.
    2. Add an entry to the gs_KuTestCases table in KNodeTableTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KNodeTableTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


struct Node {
    ULONG Number;
    KTableEntry TableEntry;
};

LONG
Compare(
    __in Node& First,
    __in Node& Second
    )
{
    if (First.Number < Second.Number) {
        return -1;
    }
    if (First.Number > Second.Number) {
        return 1;
    }
    return 0;
}

typedef KNodeTable<Node> NumberTable;

NTSTATUS
KNodeTableTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    NumberTable::CompareFunction compare(&Compare);
    NumberTable table(FIELD_OFFSET(Node, TableEntry), compare);
    Node* node;
    BOOLEAN b;
    Node searchKey;
    ULONG i;
    Node* priorNode;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KNodeTableTest test");

    //
    // Insert the even elements from 1 to 100 into the table.
    //

    for (i = 2; i <= 100; i += 2) {

        node = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) Node;

        if (!node) {
            KTestPrintf("memory allocation failure\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        node->Number = i;

        b = table.Insert(*node);

        if (!b) {
            KTestPrintf("Node %d could not be inserted\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        b = table.VerifyTable();

        if (!b) {
            KTestPrintf("Table verification failed.\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // There should be exactly 50 elements in the table.
    //

    if (table.Count() != 50) {
        KTestPrintf("There are %d elements in the table instead of 50.\n", table.Count());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Lookup an element that is expected to not be there.
    //

    searchKey.Number = 5;

    node = table.Lookup(searchKey);

    if (node) {
        KTestPrintf("A node was found when none should have been there.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Lookup an element that is expected to be there.
    //

    searchKey.Number = 10;

    node = table.Lookup(searchKey);

    if (!node) {
        KTestPrintf("No node was returned when 10 should have been returned.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (node->Number != 10) {
        KTestPrintf("Wrong node was returned.  10 expected.  Got %d instead.\n", node->Number);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Lookup an element that is not there but use 'ornext' to get the next one.
    //

    searchKey.Number = 49;

    node = table.LookupEqualOrNext(searchKey);

    if (!node) {
        KTestPrintf("No node was returned when 50 should have been returned.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (node->Number != 50) {
        KTestPrintf("Wrong node was returned.  50 expected.  Got %d instead.\n", node->Number);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Lookup an element that is there using 'ornext'.  Should get the element anyway.
    //

    searchKey.Number = 76;

    node = table.LookupEqualOrNext(searchKey);

    if (!node) {
        KTestPrintf("No node was returned when 76 should have been returned.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (node->Number != 76) {
        KTestPrintf("Wrong node was returned.  76 expected.  Got %d instead.\n", node->Number);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Lookup an element that is not there but use 'orprevious' to get the previous one.
    //

    searchKey.Number = 99;

    node = table.LookupEqualOrPrevious(searchKey);

    if (!node) {
        KTestPrintf("No node was returned when 98 should have been returned.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (node->Number != 98) {
        KTestPrintf("Wrong node was returned.  98 expected.  Got %d instead.\n", node->Number);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Lookup an element that is there using 'orprevious'.  Should get the element anyway.
    //

    searchKey.Number = 100;

    node = table.LookupEqualOrPrevious(searchKey);

    if (!node) {
        KTestPrintf("No node was returned when 100 should have been returned.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (node->Number != 100) {
        KTestPrintf("Wrong node was returned.  100 expected.  Got %d instead.\n", node->Number);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Iterate through forwards.  Count the elements, there should be 50.
    //

    i = 0;
    for (node = table.First(); node; node = table.Next(*node)) {
        i++;
    }

    if (i != 50) {
        KTestPrintf("Forward iterator only hits %d elements, instead of 50\n", i);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Iterate through backwards.  Count the elements, there should be 50.
    //

    i = 0;
    for (node = table.Last(); node; node = table.Previous(*node)) {
        i++;
    }

    if (i != 50) {
        KTestPrintf("Forward iterator only hits %d elements, instead of 50\n", i);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Iterate forwards, and delete every other element.
    //

    i = 0;
    for (node = table.First(); node; node = node ? table.Next(*node) : table.First()) {
        if (i%2 == 0) {
            priorNode = table.Previous(*node);
            table.Remove(*node);
            b = table.VerifyTable();
            if (!b) {
                KTestPrintf("Verify failed after deleting %d\n", node->Number);
                _delete(node);
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }
            _delete(node);
            node = priorNode;
        }
        i++;
    }

Finish:

    node = table.First();
    while (node) {
        table.Remove(*node);
        b = table.VerifyTable();
        if (!b) {
            KTestPrintf("Verify failed after deleting %d\n", node->Number);
            status = STATUS_INVALID_PARAMETER;
            _delete(node);
            break;
        }
        _delete(node);
        node = table.First();
    }

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KNodeTableTest(
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
    
    KTestPrintf("KNodeTableTest: STARTED\n");

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KNodeTableTestX(argc, args);

    KtlSystem::Shutdown();

    KTestPrintf("KNodeTableTest: COMPLETED\n");

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

    return RtlNtStatusToDosError(KNodeTableTest(0, NULL));
}
#endif
