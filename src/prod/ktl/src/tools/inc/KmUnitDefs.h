/*++

Copyright (c) Microsoft Corporation

Module Name:

    KmUnitDefs.h

Abstract:

    This file provides interface to the KmUnit kernel-mode driver.
    It is shared between the driver and any user-mode application.

Environment:

    Kernel mode, user mode.

--*/

#pragma once

#if KTL_USER_MODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <specstrings.h>
#include <winbase.h>
#endif

//
// Control device names.
//

#ifndef TESTNAME
#error TESTNAME must be specified
#endif

#if !defined(PLATFORM_UNIX)
#define KU_NT_CONTROL_NAME          L"\\Device\\KmUnit"TESTNAME
#define KU_DOS_CONTROL_NAME         L"\\DosDevices\\KmUnit"TESTNAME
#define KU_CONTROL_NAME             L"\\\\.\\KmUnit"
#else
#define KU_NT_CONTROL_NAME          L"\\Device\\KmUnit" TESTNAME
#define KU_DOS_CONTROL_NAME         L"\\DosDevices\\KmUnit" TESTNAME
#define KU_CONTROL_NAME             L"\\\\.\\KmUnit"
#endif

//
// Define the device type in the "user defined" range
//

#define KU_CONTROL_TYPE         (40000)

//
// The following IOCTLs are supported by the control object.
//

#define IOCTL_KU_CONTROL_QUERY_NUMBER_OF_TESTS          CTL_CODE(KU_CONTROL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_KU_CONTROL_QUERY_TESTS                    CTL_CODE(KU_CONTROL_TYPE, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_KU_CONTROL_START_TEST                     CTL_CODE(KU_CONTROL_TYPE, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// Input structure for IOCTL_KU_CONTROL_START_TEST.
//

typedef struct _KU_TEST_START {

    //
    // A numerical ID of the test returned from IOCTL_KU_CONTROL_QUERY_TESTS.
    //

    ULONG   TestId;

    //
    // Parameter to break on test start
    //

    BOOLEAN BreakOnStart;

    //
    // Optional parameters to pass to the test.
    //

    ULONG   ParameterBytes;
    BYTE    Parameter[1];
} KU_TEST_START, *PKU_TEST_START;

//
// An unique ID of a test case.
// Id is unique. Name is for annotation purpose only. It does not have to be unique.
//

#define KU_TEST_NAME_LENGTH     (64)
#define KU_MAX_PATH     (256)
#define KU_MAX_PARAMETERS     (16)
#define KU_MAX_POST_OP_LENGTH (1024)

typedef struct _KU_TEST_ID {
    ULONG   Id;
    WCHAR   Name[KU_TEST_NAME_LENGTH];
    WCHAR   Categories[KU_MAX_PATH];
    WCHAR   Help[KU_MAX_PATH];
} KU_TEST_ID, *PKU_TEST_ID;

//
// It contains an array of test IDs available in the driver.
//

typedef struct _KU_TEST_ID_ARRAY {
    ULONG           NumberOfTests;
    ULONG           Reserved1;
    KU_TEST_ID      TestIdArray[1];
} KU_TEST_ID_ARRAY, *PKU_TEST_ID_ARRAY;

typedef struct _TEST_PARAMS {
    WCHAR    Id[KU_TEST_NAME_LENGTH];
    int      Count;
    BOOLEAN  HasRun;
    WCHAR    Parameter[KU_MAX_PARAMETERS][KU_MAX_PATH];
    WCHAR    PostOpCmd[KU_MAX_POST_OP_LENGTH];
} TEST_PARAMS, *PTEST_PARAMS;

typedef struct _TEST_PARAMS_ARRAY {
    size_t           NumberOfTests;
    ULONG            Reserved1;
    TEST_PARAMS      TestParamsArray[1];
} TEST_PARAMS_ARRAY, *PTEST_PARAMS_ARRAY;
