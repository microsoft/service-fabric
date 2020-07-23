// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "WexTestClass.h"

#include <ktl.h>
#include <ktrace.h>
#include <ktllogger.h>

#include "KtlLoggerTests.h"

#define ALLOCATION_TAG 'LLT'

namespace KtlPhysicalLogTest
{
// To include tests in the usermode dll use the following
#ifndef KDRIVER1

// To exclude tests in the usermode dll use the following
//#ifdef KDRIVER
    //
    // Kernel mode only tests
    //
    class KtlLoggerTests : public WEX::TestClass<KtlLoggerTests>
    {
    public:
        TEST_CLASS(KtlLoggerTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(IoctlTest);
		TEST_METHOD(OneBitLogCorruptionTest);
        TEST_METHOD(ContainerConfigurationTest);
        TEST_METHOD(ContainerWithPathTest);
        TEST_METHOD(EnunerateStreamsTest);
        TEST_METHOD(AliasTest);

        TEST_METHOD(Close2Test)
        TEST_METHOD(LLCoalesceDataTest)
        TEST_METHOD(LLDataOverwriteTest)
        TEST_METHOD(LLRecordMarkerRecoveryTest)             
        TEST_METHOD(LLDataTruncateTest)
        TEST_METHOD(LLBarrierRecoveryTest)                
        TEST_METHOD(MultiRecordReadTest)                
        TEST_METHOD(SetConfigurationTest)
#ifndef UPASSTHROUGH
        TEST_METHOD(ConfiguredMappedLogTest)
#endif
        TEST_METHOD(ReservationsTest)

#if DBG
#ifdef KDRIVER
        TEST_METHOD(ThrottledLockedMemoryTest)              
#endif
#endif

#ifdef INCLUDE_OPEN_SPECIFIC_STREAM_TEST
        // Enable this when debugging a log recovery issue, that is
        // when a log fails to open. In this case be sure to comment
        // out the code that deletes all containers before starting
        // this test. Note that this should be build for UDRIVER (usermode)
        TEST_METHOD(OpenSpecificStreamTest)
#endif // INCLUDE_OPEN_SPECIFIC_STREAM_TEST
#endif // KDRIVER
                
    private:
        NTSTATUS KtlLoggerTests::FindDiskIdForDriveLetter(
            UCHAR& DriveLetter,
            GUID& DiskIdGuid
            );
        
                
    private:
        const static _LogManagerCount = 16;
        
        KGuid _diskId;
        UCHAR _driveLetter;
        KtlLogManager::SPtr _logManagers[_LogManagerCount];
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
    };

    void KtlLoggerTests::IoctlTest()
    {
        ::IoctlTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::ContainerConfigurationTest()
    {
        ::ContainerConfigurationTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::ContainerWithPathTest()
    {
        ::ContainerWithPathTest(_driveLetter, _logManagers[0]);
    }
    
    void KtlLoggerTests::OneBitLogCorruptionTest()
    {
        ::OneBitLogCorruptionTest(_driveLetter, _logManagers[0]);
    }
    
    void KtlLoggerTests::EnunerateStreamsTest()
    {
        ::EnunerateStreamsTest(_diskId, _logManagers[0]);
    }

   void KtlLoggerTests::AliasTest()
    {
        ::AliasTest(_diskId, _logManagers[0]);
    }   

    void KtlLoggerTests::Close2Test()
    {
        ::Close2Test(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::LLRecordMarkerRecoveryTest()
    {
        ::LLRecordMarkerRecoveryTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::LLDataOverwriteTest()
    {
        ::LLDataOverwriteTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::LLCoalesceDataTest()
    {
        ::LLCoalesceDataTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::LLBarrierRecoveryTest()
    {
        ::LLBarrierRecoveryTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::LLDataTruncateTest()
    {
        ::LLDataTruncateTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::MultiRecordReadTest()
    {
        ::MultiRecordReadTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::SetConfigurationTest()
    {
        ::SetConfigurationTest(_diskId, _logManagers[0]);
    }

#ifndef UPASSTHROUGH
    void KtlLoggerTests::ConfiguredMappedLogTest()
    {
        ::ConfiguredMappedLogTest(_driveLetter, _logManagers[0]);
    }
#endif
    
    void KtlLoggerTests::ReservationsTest()
    {
        ::ReservationsTest(_diskId, _logManagers[0]);
    }
    
#if DBG
#ifdef KDRIVER
    void KtlLoggerTests::ThrottledLockedMemoryTest()
    {
        ::ThrottledLockedMemoryTest(_diskId, _logManagers[0]);
    }
#endif
#endif
    
#ifdef INCLUDE_OPEN_SPECIFIC_STREAM_TEST
    void KtlLoggerTests::OpenSpecificStreamTest()
    {
        ::OpenSpecificStreamTest(_diskId, _logManagers[0]);
    }
#endif   // INCLUDE_OPEN_SPECIFIC_STREAM_TEST

//    void KtlLoggerTests::ManyVersionsTest()
//    {
//        ::ManyVersionsTest(_diskId, _logManagers[0]);
//    }   

    NTSTATUS KtlLoggerTests::FindDiskIdForDriveLetter(
        UCHAR& DriveLetter,
        GUID& DiskIdGuid
        )
    {
        return(::FindDiskIdForDriveLetter(DriveLetter, DiskIdGuid));
    }
    
    bool KtlLoggerTests::Setup()
    {
        ::SetupTests(_diskId,
                     _driveLetter,
                     _LogManagerCount,
                     _logManagers,
                     _startingAllocs,
                     _system);
                            
        return true;
    }

    bool KtlLoggerTests::Cleanup()
    {
        ::CleanupTests(_diskId,
                     _LogManagerCount,
                     _logManagers,
                     _startingAllocs,
                     _system);
        
        return true;
    }


    class RawKtlLoggerTests : public WEX::TestClass<RawKtlLoggerTests>
    {
    public:
        TEST_CLASS(RawKtlLoggerTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

#ifdef KDRIVER
        // Kernel mode only
        TEST_METHOD(ReuseLogManagerTest);
#endif

    private:
        KGuid _diskId;
        UCHAR _driveLetter;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
        
    };

#ifdef KDRIVER
    void RawKtlLoggerTests::ReuseLogManagerTest()
    {
        ::ReuseLogManagerTest(_diskId,
                              128);        // LogManagerCount
                              
    }
#endif
    
    bool RawKtlLoggerTests::Setup()
    {
        ::SetupRawKtlLoggerTests(_diskId,
                                 _driveLetter,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }

    bool RawKtlLoggerTests::Cleanup()
    {
        ::CleanupRawKtlLoggerTests(_diskId,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }
    

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    class OverlayLogTests : public WEX::TestClass<OverlayLogTests>
    {
    public:
        TEST_CLASS(OverlayLogTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)
                
        TEST_METHOD(VerifySharedLogUsageThrottlingTest);
        TEST_METHOD(DestagedWriteTest);
        TEST_METHOD(CoalescedWriteTest);
        TEST_METHOD(CoalescedWrite2Test);
        TEST_METHOD(RecoveryTest);
        TEST_METHOD(Recovery2Test);     
        TEST_METHOD(RetryOnSharedLogFullTest);
        TEST_METHOD(RecoveryViaOpenContainerTest);
        TEST_METHOD(DeleteTruncatedTailRecords1Test);

        TEST_METHOD(OfflineDestageContainerTest);
        TEST_METHOD(ReadFromCoalesceBuffersTest);
        
    private:
        KGuid _diskId;
        UCHAR _driveLetter;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
        
    };

    void OverlayLogTests::ReadFromCoalesceBuffersTest()
    {
        ::ReadFromCoalesceBuffersTest(_diskId);                              
    }
   
    void OverlayLogTests::VerifySharedLogUsageThrottlingTest()
    {
        ::VerifySharedLogUsageThrottlingTest(_diskId);                              
    }

    void OverlayLogTests::DestagedWriteTest()
    {
        ::DestagedWriteTest(_diskId);                              
    }

    void OverlayLogTests::CoalescedWriteTest()
    {
        ::CoalescedWriteTest(_diskId);
    }

    void OverlayLogTests::CoalescedWrite2Test()
    {
        ::CoalescedWriteTest(_diskId);
    }

    void OverlayLogTests::RecoveryTest()
    {
        ::RecoveryTest(_diskId);
                              
    }

    void OverlayLogTests::Recovery2Test()
    {
//        ::Recovery2Test(_diskId);                              
    }

    void OverlayLogTests::RetryOnSharedLogFullTest()
    {
        ::RetryOnSharedLogFullTest(_diskId);
                              
    }

    void OverlayLogTests::OfflineDestageContainerTest()
    {
        ::OfflineDestageContainerTest(_diskId);                              
    }

    void OverlayLogTests::RecoveryViaOpenContainerTest()
    {
        ::RecoveryViaOpenContainerTest(_diskId);
                              
    }   
    void OverlayLogTests::DeleteTruncatedTailRecords1Test()
    {
        ::DeleteTruncatedTailRecords1Test(_diskId);                              
    }   
    
    bool OverlayLogTests::Setup()
    {
        ::SetupOverlayLogTests(_diskId,
                               _driveLetter,
                               _startingAllocs,
                               _system);
                            
        return true;
    }

    bool OverlayLogTests::Cleanup()
    {
        ::CleanupOverlayLogTests(_diskId,
                                 _driveLetter,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }   
#endif // UDRIVER || UPASSTHROUGH
}
