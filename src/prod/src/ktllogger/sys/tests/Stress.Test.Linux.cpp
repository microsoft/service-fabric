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

#if 0 // Re-enable when package 23 is checked in
    BOOST_AUTO_TEST_CASE(Stress1Test)
    {
        ::Stress1Test(_diskId, _LogManagerCount, _logManagers);
    }
#endif

    BOOST_AUTO_TEST_CASE(AliasStress)
    {
        ::AliasStress(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(LLDataWorkloadTest)
    {
        ::LLDataWorkloadTest(_diskId, _logManagers[0]);
    }

#if 0   // Reenable later after shipping 2.2
    BOOST_AUTO_TEST_CASE(ServiceWrapperStressTest()
    {
        ::ServiceWrapperStressTest(_diskId, _logManagers[0]);
    }
#endif // 0

#if 0  // Re-enable when able to run on cloud
    BOOST_AUTO_TEST_CASE(AccelerateFlushTest)
    {
        ::AccelerateFlushTest(_diskId, _logManagers[0]);
    }
#endif
    
    BOOST_AUTO_TEST_CASE(ContainerLimitsTest)
    {
        ::ContainerLimitsTest(_diskId, _logManagers[0]);
    }

    BOOST_AUTO_TEST_CASE(WriteTruncateAndRecoverTest)
    {
        ::WriteTruncateAndRecoverTest(_driveLetter, _logManagers[0]);
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
}
