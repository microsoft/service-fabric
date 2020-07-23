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
    
#if DBG
#ifdef KDRIVER
    BOOST_AUTO_TEST_CASE(ThrottledLockedMemoryTest)
    {
        ::ThrottledLockedMemoryTest(_diskId, _logManagers[0]);
    }
#endif
#endif
    
#ifdef INCLUDE_OPEN_SPECIFIC_STREAM_TEST
    BOOST_AUTO_TEST_CASE(OpenSpecificStreamTest)
    {
        ::OpenSpecificStreamTest(_diskId, _logManagers[0]);
    }
#endif   // INCLUDE_OPEN_SPECIFIC_STREAM_TEST

    BOOST_AUTO_TEST_CASE(ReservationsTest)
    {
        ::ReservationsTest(_diskId, _logManagers[0]);
    }
    
    BOOST_AUTO_TEST_CASE(IoctlTest)
    {
        ::IoctlTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ContainerConfigurationTest)
    {
        ::ContainerConfigurationTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(ContainerWithPathTest)
    {
        ::ContainerWithPathTest(_driveLetter, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(OneBitLogCorruptionTest)
    {
        ::OneBitLogCorruptionTest(_driveLetter, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(EnunerateStreamsTest)
    {
        ::EnunerateStreamsTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(AliasTest)
    {
        ::AliasTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(Close2Test)
    {
        ::Close2Test(_diskId, _logManagers[0]);
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

    BOOST_AUTO_TEST_CASE(MultiRecordReadTest)
    {
        ::MultiRecordReadTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_SUITE_END()           
            
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
    
    BOOST_AUTO_TEST_SUITE_END()


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
//        ::Recovery2Test(_diskId);
    }

    BOOST_AUTO_TEST_CASE(RecoveryViaOpenContainerTest)
    {
        ::RecoveryViaOpenContainerTest(_diskId);

    }
    BOOST_AUTO_TEST_CASE(RecoveryPartlyCreatedStreamTest)
    {
        ::RecoveryPartlyCreatedStreamTest(_diskId);

    }

    BOOST_AUTO_TEST_CASE(DeleteTruncatedTailRecords1Test)
    {
        ::DeleteTruncatedTailRecords1Test(_diskId);
    }
    
    BOOST_AUTO_TEST_CASE(OfflineDestageContainerTest)
    {
        ::OfflineDestageContainerTest(_diskId);
    }

    BOOST_AUTO_TEST_CASE(ReadFromCoalesceBuffersTest)
    {
        ::ReadFromCoalesceBuffersTest(_diskId);
    }

    BOOST_AUTO_TEST_SUITE_END()

#endif // UDRIVER

}
