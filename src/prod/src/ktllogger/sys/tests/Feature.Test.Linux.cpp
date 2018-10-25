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


    BOOST_AUTO_TEST_CASE(CorruptedLCMBInfoTest)
    {
        ::CorruptedLCMBInfoTest(_driveLetter, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(PartialContainerCreateTest)
    {
        ::PartialContainerCreateTest(_driveLetter, _logManagers[0]);
    }

#if 0  // TODO: Re-enable when this can be made more reliable
    BOOST_AUTO_TEST_CASE(VerifySparseTruncateTest)
    {
        ::VerifySparseTruncateTest(_driveLetter, _logManagers[0]);
    }
#endif

    BOOST_AUTO_TEST_CASE(DeletedDedicatedLogTest)
    {
        ::DeletedDedicatedLogTest(_diskId, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(EnumContainersTest)
    {
        ::EnumContainersTest(_diskId, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(StreamMetadataTest)
    {
        ::StreamMetadataTest(_diskId, _logManagers[0]);
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

    BOOST_AUTO_TEST_CASE(ManyKIoBufferElementsTest)
    {
        ::ManyKIoBufferElementsTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(CloseTest)
    {
        ::CloseTest(_diskId, _logManagers[0]);
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

    BOOST_AUTO_TEST_CASE(DLogReservationTest)
    {
        ::DLogReservationTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_SUITE_END()

            
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

    BOOST_AUTO_TEST_CASE(DuplicateRecordInLogTest)
    {
        ::DuplicateRecordInLogTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(CorruptedRecordTest)
    {
        ::CorruptedRecordTest(_diskId);
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
								   _driveLetter,
                                   _startingAllocs,
                                   _system);

            return true;
        }

        bool Cleanup()
        {
            ::CleanupOverlayLogTests(_diskId,
 								     _driveLetter,
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
        UCHAR _driveLetter;		
        ULONGLONG _startingAllocs;
        KtlSystem* _system;

    };

    BOOST_FIXTURE_TEST_SUITE(OverlayLogTestsSuite, OverlayLogTests)

    BOOST_AUTO_TEST_CASE(PeriodicTimerCloseRaceTest)
    {
        ::PeriodicTimerCloseRaceTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(DiscontiguousRecordsRecoveryTest)
    {
        ::DiscontiguousRecordsRecoveryTest(_diskId);
    }
	
    BOOST_AUTO_TEST_CASE(RecoveryPartlyCreatedStreamTest)
    {
        ::RecoveryPartlyCreatedStreamTest(_diskId);

    }

    BOOST_AUTO_TEST_CASE(StreamQuotaTest)
    {
        ::StreamQuotaTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(RetryOnSharedLogFullTest)
    {
        ::StreamQuotaTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ThrottledAllocatorTest)
    {
        ::ThrottledAllocatorTest(_diskId);
    }
   
    BOOST_AUTO_TEST_CASE(VerifyCopyFromSharedToBackupTest)
    {
        ::VerifyCopyFromSharedToBackupTest(_driveLetter, _diskId);
    }

    BOOST_AUTO_TEST_CASE(VerifySharedTruncateOnDedicatedFailureTest)
    {
        ::VerifySharedTruncateOnDedicatedFailureTest(_diskId);
    }
	
    BOOST_AUTO_TEST_CASE(FlushAllRecordsForCloseWaitTest)
    {
        ::FlushAllRecordsForCloseWaitTest(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(WriteStuckConditionsTest)
    {
        ::WriteStuckConditionsTest(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(WriteThrottleUnitTest)
    {
        ::WriteThrottleUnitTest(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(ReadFromCoalesceBuffersCornerCase1Test)
    {
        ::ReadFromCoalesceBuffersCornerCase1Test(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ReadFromCoalesceBuffersTruncateTailTest)
    {
        ::ReadFromCoalesceBuffersTruncateTailTest(_diskId);
    }

#if 0   // Enable when ktl package 23 is checked in
    BOOST_AUTO_TEST_CASE(ReadWriteRaceTest)
    {
        ::ReadWriteRaceTest(_diskId);
    }
#endif
    BOOST_AUTO_TEST_SUITE_END()

#endif // UDRIVER

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
