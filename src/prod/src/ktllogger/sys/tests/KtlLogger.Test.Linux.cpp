// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE KtlLoggerTest
#include <boost/test/unit_test.hpp>
#include "boost-taef.h"

#include "Stdafx.h"

#include <ktl.h>
#include <ktrace.h>
#include <ktllogger.h>

#include "KtlLoggerTests.h"

#include "RWTStress.h"

#define ALLOCATION_TAG 'LLT'

namespace KtlPhysicalLogTest
{
    // All tests run in User mode
    
    //
    // Kernel mode only tests
    //
    class KtlLoggerTests
    {
    public:
        KtlLoggerTests()
        {
            Setup();
        }

        ~KtlLoggerTests()
        {
            Cleanup();
        }

        NTSTATUS FindDiskIdForDriveLetter(
                UCHAR& DriveLetter,
                GUID& DiskIdGuid
        )
        {
            return(::FindDiskIdForDriveLetter(DriveLetter, DiskIdGuid));
        }

        bool Setup()
        {
            ::SetupTests(_diskId,
                         _driveLetter,
                         _LogManagerCount,
                         _logManagers,
                         _startingAllocs,
                         _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupTests(_diskId,
                           _LogManagerCount,
                           _logManagers,
                           _startingAllocs,
                           _system);

            return true;
        }

    public:
        const static int _LogManagerCount = 16;

        KGuid _diskId;
        UCHAR _driveLetter;
        KtlLogManager::SPtr _logManagers[_LogManagerCount];
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
    };

    BOOST_FIXTURE_TEST_SUITE(KtlLoggerTestsSuite, KtlLoggerTests)

    BOOST_AUTO_TEST_CASE(VersionTest)
    {
        ::VersionTest(_diskId, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(PartialContainerCreateTest)
    {
        ::PartialContainerCreateTest(_driveLetter, _logManagers[0]);
    }

#if 0 // Re-enable when package 23 is checked in
    BOOST_AUTO_TEST_CASE(Stress1Test)
    {
        ::Stress1Test(_diskId, _LogManagerCount, _logManagers);
    }
#endif

    BOOST_AUTO_TEST_CASE(SetConfigurationTest)
    {
        ::SetConfigurationTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(LLCoalesceDataTest)
    {
        ::LLCoalesceDataTest(_diskId, _logManagers[0]);
    }

#ifndef UPASSTHROUGH
    BOOST_AUTO_TEST_CASE(ConfiguredMappedLogTest)
    {
        ::ConfiguredMappedLogTest(_driveLetter, _logManagers[0]);
    }
#endif
    
#ifdef FEATURE_TEST

#if 0  // TODO: Re-enable when this can be made more reliable
    BOOST_AUTO_TEST_CASE(VerifySparseTruncateTest)
    {
        ::VerifySparseTruncateTest(_driveLetter, _logManagers[0]);
    }
#endif
    
    BOOST_AUTO_TEST_CASE(EnumContainersTest)
    {
        ::EnumContainersTest(_diskId, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(OpenCloseStreamTest)
    {
        ::OpenCloseStreamTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(AliasStress)
    {
        ::AliasStress(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(RWTStress)
    {
        ::RWTStress(KtlSystem::GlobalNonPagedAllocator(), _diskId, _logManagers[0],
                    16,    // NumberAsyncs
                    4,    // RepeatCount
                    4,        // NumberRecords
                    16 * 0x1000);
    }

    BOOST_AUTO_TEST_CASE(LLDataWorkloadTest)
    {
        ::LLDataWorkloadTest(_diskId, _logManagers[0]);
    }

#if DBG
#ifdef KDRIVER
    BOOST_AUTO_TEST_CASE(ThrottledLockedMemoryTest)
    {
        ::ThrottledLockedMemoryTest(_diskId, _logManagers[0]);
    }
#endif
#endif
    
#if 0   // Reenable later after shipping 2.2
    BOOST_AUTO_TEST_CASE(ServiceWrapperStressTest()
    {
        ::ServiceWrapperStressTest(_diskId, _logManagers[0]);
    }
#endif // 0

#endif // FEATURE_TEST

#ifdef INCLUDE_OPEN_SPECIFIC_STREAM_TEST
    BOOST_AUTO_TEST_CASE(OpenSpecificStreamTest)
    {
        ::OpenSpecificStreamTest(_diskId, _logManagers[0]);
    }
#endif   // INCLUDE_OPEN_SPECIFIC_STREAM_TEST

//    BOOST_AUTO_TEST_CASE(ManyVersionsTest)
//    {
//        ::ManyVersionsTest(_diskId, _logManagers[0]);
//    }
    
    BOOST_AUTO_TEST_CASE(ReservationsTest)
    {
        ::ReservationsTest(_diskId, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(IoctlTest)
    {
        ::IoctlTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ManagerTest)
    {
        ::ManagerTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ContainerTest)
    {
        ::ContainerTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(MissingStreamFileTest)
    {
        ::MissingStreamFileTest(_driveLetter, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ContainerConfigurationTest)
    {
        ::ContainerConfigurationTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ContainerWithPathTest)
    {
        ::ContainerWithPathTest(_driveLetter, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(StreamTest)
    {
        ::StreamTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(EnunerateStreamsTest)
    {
        ::EnunerateStreamsTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ReadAsnTest)
    {
        ::ReadAsnTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(StreamMetadataTest)
    {
        ::StreamMetadataTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(AliasTest)
    {
        ::AliasTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(CloseOpenRaceTest)
    {
        ::CloseOpenRaceTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(OpenRightAfterCloseTest)
    {
        ::OpenRightAfterCloseTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(QueryRecordsTest)
    {
        ::QueryRecordsTest(_diskId, _logManagers[0]);
    }
    
#if defined(UDRIVER) || defined(UPASSTHROUGH)
    BOOST_AUTO_TEST_CASE(OpenWriteCloseTest)
    {
        ::OpenWriteCloseTest(_diskId, _logManagers[0]);
    }
#endif  
#ifdef UDRIVER
    BOOST_AUTO_TEST_CASE(FailCoalesceFlushTest)
    {
        ::FailCoalesceFlushTest(_diskId, _logManagers[0]);
    }
#endif

    BOOST_AUTO_TEST_CASE(ForcePreAllocReallocTest)
    {
        ::ForcePreAllocReallocTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ManyKIoBufferElementsTest)
    {
        ::ManyKIoBufferElementsTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(CloseTest)
    {
        ::CloseTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(Close2Test)
    {
        ::Close2Test(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ThresholdTest)
    {
        ::ThresholdTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(CancelStressTest)
    {
        ::CancelStressTest(2,       // Number loops
                           64,      // Number Asyncs
                           _diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(DeleteOpenStreamTest)
    {
        ::DeleteOpenStreamTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ContainerLimitsTest)
    {
        ::ContainerLimitsTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(LLDataOverwriteTest)
    {
        ::LLDataOverwriteTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(LLRecordMarkerRecoveryTest)
    {
        ::LLRecordMarkerRecoveryTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(LLDataTruncateTest)
    {
        ::LLDataTruncateTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(LLBarrierRecoveryTest)
    {
        ::LLBarrierRecoveryTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(DLogReservationTest)
    {
        ::DLogReservationTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(DeletedDedicatedLogTest)
    {
        ::DeletedDedicatedLogTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(WriteTruncateAndRecoverTest)
    {
        ::WriteTruncateAndRecoverTest(_driveLetter, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(MultiRecordReadTest)
    {
        ::MultiRecordReadTest(_diskId, _logManagers[0]);
    }


    BOOST_AUTO_TEST_SUITE_END()

            
#ifdef FEATURE_TEST
#if defined(UDRIVER) || defined(UPASSTHROUGH)
    class MetadataBlockInfoTests
    {
    public:
        MetadataBlockInfoTests()
        {
            Setup();
        }

        ~MetadataBlockInfoTests()
        {
            Cleanup();
        }

        bool Setup()
        {
            ::SetupMBInfoTests(_diskId,
                               _startingAllocs,
                               _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupMBInfoTests(_diskId,
                               _startingAllocs,
                                 _system);

            return true;
        }

    public:
        KGuid _diskId;
        KtlSystem* _system;
        ULONGLONG _startingAllocs;
    };

    BOOST_FIXTURE_TEST_SUITE(MetadataBlockInfoTestsSuite, MetadataBlockInfoTests)

    BOOST_AUTO_TEST_CASE(BasicMBInfoTest)
    {
        ::BasicMBInfoTest(_diskId,
                          1,              // Loops
                          16,             // Number blocks
                          0x4000);        // Block Size
    }

    BOOST_AUTO_TEST_CASE(BasicMBInfoDataIntegrityTest)
    {
        ::BasicMBInfoDataIntegrityTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(FailOpenCorruptedMBInfoTest)
    {
        ::FailOpenCorruptedMBInfoTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(SharedLCMBInfoTest)
    {
        ::SharedLCMBInfoTest(_diskId,
                          1,              // Loops
                          0x800);         // Number entries
    }

    BOOST_AUTO_TEST_CASE(DedicatedLCMBInfoTest)
    {
        ::DedicatedLCMBInfoTest(_diskId,
                                1);              // Loops
    }
    
    BOOST_AUTO_TEST_SUITE_END()
#endif  // UDRIVER
#endif  // FEATURE_TEST

            
    //
    // Kernel mode only
    //
    class RawKtlLoggerTests
    {
    public:
        KGuid _diskId;
        UCHAR _driveLetter;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;

        RawKtlLoggerTests()
        {
            Setup();
        }

        ~RawKtlLoggerTests()
        {
            Cleanup();
        }

        bool Setup()
        {
            ::SetupRawKtlLoggerTests(_diskId,
                                     _driveLetter,
                                     _startingAllocs,
                                     _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupRawKtlLoggerTests(_diskId,
                                       _startingAllocs,
                                       _system);

            return true;
        }       
    };

    BOOST_FIXTURE_TEST_SUITE(RawKtlLoggerTestsSuite, RawKtlLoggerTests)

            
#ifdef KDRIVER
    BOOST_AUTO_TEST_CASE(ReuseLogManagerTest)
    {
        ::ReuseLogManagerTest(_diskId,
                              128);        // LogManagerCount
    }
#endif

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    BOOST_AUTO_TEST_CASE(CreateLogContainerWithBadPathTest)
    {
        ::CreateLogContainerWithBadPathTest(_driveLetter);

    }
#endif

    BOOST_AUTO_TEST_CASE(OpenCloseLogManagerStressTest)
    {
        ::OpenCloseLogManagerStressTest();

    }

    
    BOOST_AUTO_TEST_SUITE_END()

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    //
    // RAW Core logger tests
    //
    class RawCoreLoggerTests
    {
    public:
        KGuid _diskId;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;

    public:
        RawCoreLoggerTests()
        {
            Setup();
        }

        ~RawCoreLoggerTests()
        {
            Cleanup();
        }

        bool Setup()
        {
            ::SetupRawCoreLoggerTests(_diskId,
                                      _startingAllocs,
                                      _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupRawCoreLoggerTests(_diskId,
                                        _startingAllocs,
                                        _system);

            return true;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(RawCoreLoggerTestsSuite, RawCoreLoggerTests)
            
    BOOST_AUTO_TEST_CASE(TruncateToBadLogLowLsnTest)
    {
        ::TruncateToBadLogLowLsnTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(StreamCheckpointBoundaryTest)
    {
        ::StreamCheckpointBoundaryTest(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(QueryLogTypeTest)
    {
        ::QueryLogTypeTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(DeleteRecordsTest)
    {
        ::DeleteRecordsTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(StreamCheckpointAtEndOfLogTest)
    {
        ::StreamCheckpointAtEndOfLogTest(_diskId);
    }
    
    BOOST_AUTO_TEST_SUITE_END()
#endif // UDRIVER


#if defined(UDRIVER) || defined(UPASSTHROUGH)
    class OverlayLogTests
    {
    public:
        OverlayLogTests()
        {
            Setup();
        }

        ~OverlayLogTests()
        {
            Cleanup();
        }

        bool Setup()
        {
            ::SetupOverlayLogTests(_diskId,
                                   _startingAllocs,
                                   _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupOverlayLogTests(_diskId,
                                     _startingAllocs,
                                     _system);

            return true;
        }

    public:
        //
        // Disable these for now as the implementation of
        // MultiRecordRead has changed and these scenarios will not be
        // hit. At some point we may want to delete them, but not yet.
        //
        KGuid _diskId;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;

    };

    BOOST_FIXTURE_TEST_SUITE(OverlayLogTestsSuite, OverlayLogTests)

    BOOST_AUTO_TEST_CASE(ReadTest)
    {
        ::ReadTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(DestagedWriteTest)
    {
        ::DestagedWriteTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(CoalescedWriteTest)
    {
        ::CoalescedWriteTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(CoalescedWrite2Test)
    {
        ::CoalescedWrite2Test(_diskId);
    }

    BOOST_AUTO_TEST_CASE(RecoveryTest)
    {
        ::RecoveryTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(Recovery2Test)
    {
        ::Recovery2Test(_diskId);

    }

    BOOST_AUTO_TEST_CASE(RecoveryViaOpenContainerTest)
    {
        ::RecoveryViaOpenContainerTest(_diskId);

    }
    BOOST_AUTO_TEST_CASE(RecoveryPartlyCreatedStreamTest)
    {
        ::RecoveryPartlyCreatedStreamTest(_diskId);

    }

    BOOST_AUTO_TEST_CASE(StreamQuotaTest)
    {
        ::StreamQuotaTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(DeleteTruncatedTailRecords1Test)
    {
        ::DeleteTruncatedTailRecords1Test(_diskId);
    }
    BOOST_AUTO_TEST_CASE(ThrottledAllocatorTest)
    {
        ::ThrottledAllocatorTest(_diskId);
    }
    
#if 0 // TODO: Race conditions when reading from Coalesce and Dedicated
    BOOST_AUTO_TEST_CASE(MultiRecordCornerCaseTest)
    {
        ::MultiRecordCornerCaseTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(MultiRecordCornerCase2Test)
    {
        ::MultiRecordCornerCase2Test(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ReadRaceConditionsTest)
    {
        ::ReadRaceConditionsTest(_diskId);
    }
#endif

    BOOST_AUTO_TEST_CASE(OfflineDestageContainerTest)
    {
        ::OfflineDestageContainerTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(WriteStuckConditionsTest)
    {
        ::WriteStuckConditionsTest(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(WriteThrottleUnitTest)
    {
        ::WriteThrottleUnitTest(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(ReadFromCoalesceBuffersTest)
    {
        ::ReadFromCoalesceBuffersTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ReadFromCoalesceBuffersCornerCase1Test)
    {
        ::ReadFromCoalesceBuffersCornerCase1Test(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ReadFromCoalesceBuffersTruncateTailTest)
    {
        ::ReadFromCoalesceBuffersTruncateTailTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ReadWriteRaceTest)
    {
        ::ReadWriteRaceTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(PeriodicTimerCloseRaceTest)
    {
        ::PeriodicTimerCloseRaceTest(_diskId);
    }

    BOOST_AUTO_TEST_SUITE_END()

#endif // UDRIVER


#if defined(UDRIVER) || defined(UPASSTHROUGH)
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
    
#ifdef FEATURE_TEST
    BOOST_AUTO_TEST_CASE(RecoveryTest)
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::RvdLoggerRecoveryTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
#endif

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

#endif

#ifdef UDRIVER
    //
    // NOTE: This MUST be the last test in the list as the test does not cleanup properly, by design
    //
    class KtlLoggerMiscTests
    {
    public:
        KtlLoggerMiscTests()
        {
            Setup();
        }

        ~KtlLoggerMiscTests()
        {
            Cleanup();
        }

        bool Setup()
        {
            ::SetupMiscTests(_diskId,
                             _driveLetter,
                             _startingAllocs,
                             _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupMiscTests(_diskId,
                               _startingAllocs,
                               _system);

            return true;
        }

    public:
        KGuid _diskId;
        UCHAR _driveLetter;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
    };

    BOOST_FIXTURE_TEST_SUITE(KtlLoggerMiscTestsSuite, KtlLoggerMiscTests)

    BOOST_AUTO_TEST_CASE(ForceCloseTest)
    {
        ::ForceCloseTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ForceCloseOnDeleteContainerWaitTest)
    {
        ::ForceCloseOnDeleteContainerWaitTest(_diskId);
    }
    BOOST_AUTO_TEST_SUITE_END()
#endif
}
