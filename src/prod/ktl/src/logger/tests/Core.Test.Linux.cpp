//------------------------------------------------------------
// Copyright (c) 2012 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE KtlLoggerTest
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"

#include <ktl.h>
#include <ktrace.h>

#include "RvdLoggerTests.h"
//#include "KtlLoggerTests.h"

#define ALLOCATION_TAG 'LLT'

KAllocator* g_Allocator;


VOID SetupRawKtlLoggerTests(
    KGuid& DiskId,
    UCHAR& DriveLetter,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    NTSTATUS status;

    status = KtlSystem::Initialize(FALSE,     // Do not enable VNetwork, we don't need it
                                   &System);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    System->SetStrictAllocationChecks(TRUE);

    StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    KDbgMirrorToDebugger = TRUE;
    KDbgMirrorToDebugger = 1;
    
    EventRegisterMicrosoft_Windows_KTL();

#if !defined(PLATFORM_UNIX)
    DriveLetter = 0;
    status = FindDiskIdForDriveLetter(DriveLetter, DiskId);
    VERIFY_IS_TRUE(NT_SUCCESS(status));
#else
    // {204F01E4-6DEC-48fe-8483-B2C6B79ABECB}
    GUID randomGuid = 
        { 0x204f01e4, 0x6dec, 0x48fe, { 0x84, 0x83, 0xb2, 0xc6, 0xb7, 0x9a, 0xbe, 0xcb } };
    
    DiskId = randomGuid;    
#endif
}

VOID CleanupRawKtlLoggerTests(
    KGuid& DiskId,
    ULONGLONG& StartingAllocs,
    KtlSystem*& System
    )
{
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(System);
    UNREFERENCED_PARAMETER(StartingAllocs);

    EventUnregisterMicrosoft_Windows_KTL();

    KtlSystem::Shutdown();
}

namespace KtlPhysicalLogTest
{
    class KtlPhysicalLogTest
    {
    public:
        WCHAR _driveLetter[2];

    public:
        KtlPhysicalLogTest()
        {
            Setup();
        }

        ~KtlPhysicalLogTest()
        {
            Cleanup();
        }

        bool Setup()
        {
            WCHAR systemDrive[32];
            ULONG result = ExpandEnvironmentStringsW(L"%SYSTEMDRIVE%", (LPWSTR)systemDrive, 32);
            VERIFY_ARE_NOT_EQUAL((ULONG)0, result, L"Failed to get systemdrive");
            VERIFY_IS_TRUE(result <= 32, L"Failed to get systemdrive");

            _driveLetter[0] = systemDrive[0];
            _driveLetter[1] = 0;

            return true;
        }

        bool Cleanup()
        {
            return true;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(KtlPhysicalLogTestSuite, KtlPhysicalLogTest)

    BOOST_AUTO_TEST_CASE(BasicDiskLoggerTest)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::BasicDiskLoggerTest(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"KTL Logger Testing must be run at elevated privs");
    }

    BOOST_AUTO_TEST_CASE(AliasTest)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::RvdLoggerAliasTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"KTL Logger Testing must be run at elevated privs");
    }

    BOOST_AUTO_TEST_CASE(StreamTest)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::LogStreamAsyncIoTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KGuid diskId;
        ULONGLONG startingAllocs;
        KtlSystem* system;
        UCHAR dl;
        SetupRawKtlLoggerTests(diskId, dl, startingAllocs, system);
        CleanupRawKtlLoggerTests(diskId, startingAllocs, system);
        // Side effect is to delete all log files
    }
    
    BOOST_AUTO_TEST_CASE(RecoveryTest)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::RvdLoggerRecoveryTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }

    BOOST_AUTO_TEST_CASE(StructVerify)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::DiskLoggerStructureVerifyTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KGuid diskId;
        ULONGLONG startingAllocs;
        KtlSystem* system;
        UCHAR dl;
        SetupRawKtlLoggerTests(diskId, dl, startingAllocs, system);
        CleanupRawKtlLoggerTests(diskId, startingAllocs, system);
        // Side effect is to delete all log files
    }

    BOOST_AUTO_TEST_CASE(ReservationsTest)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::RvdLoggerReservationTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        KGuid diskId;
        ULONGLONG startingAllocs;
        KtlSystem* system;
        UCHAR dl;
        SetupRawKtlLoggerTests(diskId, dl, startingAllocs, system);
        CleanupRawKtlLoggerTests(diskId, startingAllocs, system);
        // Side effect is to delete all log files
    }

    BOOST_AUTO_TEST_SUITE_END()

}

