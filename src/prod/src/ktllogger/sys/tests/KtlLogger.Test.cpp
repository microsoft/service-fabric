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

#include "RWTStress.h"

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

        TEST_METHOD(VersionTest);
        TEST_METHOD(IoctlTest);
        TEST_METHOD(PartialContainerCreateTest);        
        TEST_METHOD(ManagerTest);
        TEST_METHOD(ContainerTest);
        TEST_METHOD(MissingStreamFileTest);     
        TEST_METHOD(ContainerConfigurationTest);
        TEST_METHOD(ContainerWithPathTest);
        TEST_METHOD(StreamTest);
        TEST_METHOD(EnunerateStreamsTest);
        TEST_METHOD(ReadAsnTest);
        TEST_METHOD(StreamMetadataTest);
        TEST_METHOD(AliasTest);

        TEST_METHOD(CloseOpenRaceTest);
        TEST_METHOD(OpenRightAfterCloseTest);

        TEST_METHOD(QueryRecordsTest)
#if defined(UDRIVER) || defined(UPASSTHROUGH)
        TEST_METHOD(OpenWriteCloseTest);
#endif
#if defined(UDRIVER)        
        TEST_METHOD(FailCoalesceFlushTest)
#endif                
        TEST_METHOD(ForcePreAllocReallocTest)
        TEST_METHOD(ManyKIoBufferElementsTest)
        TEST_METHOD(CloseTest)
        TEST_METHOD(Close2Test)
        TEST_METHOD(ThresholdTest)
        TEST_METHOD(CancelStressTest)
        TEST_METHOD(DeleteOpenStreamTest)
        TEST_METHOD(LLCoalesceDataTest)
        TEST_METHOD(LLDataOverwriteTest)
        TEST_METHOD(LLRecordMarkerRecoveryTest)             
        TEST_METHOD(LLDataTruncateTest)
        TEST_METHOD(LLBarrierRecoveryTest)                
        TEST_METHOD(DLogReservationTest)

        TEST_METHOD(DeletedDedicatedLogTest)

        TEST_METHOD(WriteTruncateAndRecoverTest)
                
        TEST_METHOD(MultiRecordReadTest)
                
        TEST_METHOD(SetConfigurationTest)

        TEST_METHOD(ContainerLimitsTest);

#ifndef UPASSTHROUGH
        TEST_METHOD(ConfiguredMappedLogTest)
#endif
//
// BUGBUG: Core logger does not truncate old versions of records when a
// new version is written. Re-enable this test when fixed
//        TEST_METHOD(ManyVersionsTest)


#ifdef FEATURE_TEST
// Only run these for feature tests                
        TEST_METHOD(EnumContainersTest)
        TEST_METHOD(OpenCloseStreamTest);

        TEST_METHOD(Stress1Test)
        TEST_METHOD(AliasStress);
        TEST_METHOD(LLDataWorkloadTest)

        TEST_METHOD(ReservationsTest)

#if DBG
#ifdef KDRIVER
        TEST_METHOD(ThrottledLockedMemoryTest)              
#endif
#endif

#if 0  // TODO: Reenable when this can be made more reliable
        TEST_METHOD(VerifySparseTruncateTest)
#endif
                
// Disable for now - re-enable after shipping 2.2               
//        TEST_METHOD(ServiceWrapperStressTest)
#endif  // FEATURE_TEST

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

    void KtlLoggerTests::ManagerTest()
    {
        ::ManagerTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::IoctlTest()
    {
        ::IoctlTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::VersionTest()
    {
        ::VersionTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::MissingStreamFileTest()
    {
        ::MissingStreamFileTest(_driveLetter, _logManagers[0]);
    }
    
    void KtlLoggerTests::ContainerTest()
    {
        ::ContainerTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::ContainerConfigurationTest()
    {
        ::ContainerConfigurationTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::ContainerWithPathTest()
    {
        ::ContainerWithPathTest(_driveLetter, _logManagers[0]);
    }
    
    void KtlLoggerTests::PartialContainerCreateTest()
    {
        ::PartialContainerCreateTest(_driveLetter, _logManagers[0]);
    }
    
    void KtlLoggerTests::StreamTest()
    {
        ::StreamTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::EnunerateStreamsTest()
    {
        ::EnunerateStreamsTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::ReadAsnTest()
    {
        ::ReadAsnTest(_diskId, _logManagers[0]);
    }   

    void KtlLoggerTests::StreamMetadataTest()
    {
        ::StreamMetadataTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::CloseOpenRaceTest()
    {
        ::CloseOpenRaceTest(_diskId, _logManagers[0]);
    }   

    void KtlLoggerTests::OpenRightAfterCloseTest()
    {
        ::OpenRightAfterCloseTest(_diskId, _logManagers[0]);
    }   

   void KtlLoggerTests::AliasTest()
    {
        ::AliasTest(_diskId, _logManagers[0]);
    }   

    void KtlLoggerTests::QueryRecordsTest()
    {
        ::QueryRecordsTest(_diskId, _logManagers[0]);
    }

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    void KtlLoggerTests::OpenWriteCloseTest()
    {
        ::OpenWriteCloseTest(_diskId, _logManagers[0]);
    }
#endif

#if defined(UDRIVER)
    void KtlLoggerTests::FailCoalesceFlushTest()
    {
        ::FailCoalesceFlushTest(_diskId, _logManagers[0]);
    }
#endif
    
    void KtlLoggerTests::ForcePreAllocReallocTest()
    {
        ::ForcePreAllocReallocTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::ManyKIoBufferElementsTest()
    {
        ::ManyKIoBufferElementsTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::ThresholdTest()
    {
        ::ThresholdTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::CancelStressTest()
    {
        ::CancelStressTest(2,       // Number loops
                           64,      // Number Asyncs
                           _diskId, _logManagers[0]);
    }

    void KtlLoggerTests::ContainerLimitsTest()
    {
        ::ContainerLimitsTest(_diskId, _logManagers[0]);
    }    

    void KtlLoggerTests::CloseTest()
    {
        ::CloseTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::Close2Test()
    {
        ::Close2Test(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::DeleteOpenStreamTest()
    {
        ::DeleteOpenStreamTest(_diskId, _logManagers[0]);
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

    void KtlLoggerTests::DLogReservationTest()
    {
        ::DLogReservationTest(_diskId, _logManagers[0]);
    }
        
    void KtlLoggerTests::DeletedDedicatedLogTest()
    {
        ::DeletedDedicatedLogTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::WriteTruncateAndRecoverTest()
    {
        ::WriteTruncateAndRecoverTest(_driveLetter, _logManagers[0]);
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
    
#ifdef FEATURE_TEST 
#if 0  // TODO: Reenable when this can be made more reliable
    void KtlLoggerTests::VerifySparseTruncateTest()
    {
        ::VerifySparseTruncateTest(_driveLetter, _logManagers[0]);
    }
#endif

    void KtlLoggerTests::OpenCloseStreamTest()
    {
        ::OpenCloseStreamTest(_diskId, _logManagers[0]);
    }   

    void KtlLoggerTests::AliasStress()
    {
        ::AliasStress(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::ReservationsTest()
    {
        ::ReservationsTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::Stress1Test()
    {
        ::Stress1Test(_diskId, _LogManagerCount, _logManagers);
    }
    
    void KtlLoggerTests::LLDataWorkloadTest()
    {
        ::LLDataWorkloadTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::EnumContainersTest()
    {
        ::EnumContainersTest(_diskId, _logManagers[0]);
    }

#if DBG
#ifdef KDRIVER
    void KtlLoggerTests::ThrottledLockedMemoryTest()
    {
        ::ThrottledLockedMemoryTest(_diskId, _logManagers[0]);
    }
#endif
#endif
    
#if 0   // Reenable later after shipping 2.2
    void KtlLoggerTests::ServiceWrapperStressTest()
    {
        ::ServiceWrapperStressTest(_diskId, _logManagers[0]);
    }
#endif // 0 

#endif // FEATURE_TEST

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


#ifdef FEATURE_TEST
#if defined(UDRIVER) || defined(UPASSTHROUGH)
    class MetadataBlockInfoTests : public WEX::TestClass<MetadataBlockInfoTests>
    {
    public:
        TEST_CLASS(MetadataBlockInfoTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(BasicMBInfoTest);
        TEST_METHOD(BasicMBInfoDataIntegrityTest);
        TEST_METHOD(FailOpenCorruptedMBInfoTest);
        TEST_METHOD(SharedLCMBInfoTest);
        TEST_METHOD(DedicatedLCMBInfoTest);
        
    private:
        KGuid _diskId;
        KtlSystem* _system;
        ULONGLONG _startingAllocs;
    };

    void MetadataBlockInfoTests::BasicMBInfoTest()
    {
        ::BasicMBInfoTest(_diskId,
                          1,              // Loops
                          16,             // Number blocks
                          0x4000);        // Block Size
    }

    void MetadataBlockInfoTests::BasicMBInfoDataIntegrityTest()
    {
        ::BasicMBInfoDataIntegrityTest(_diskId);
    }
    
    void MetadataBlockInfoTests::FailOpenCorruptedMBInfoTest()
    {
        ::FailOpenCorruptedMBInfoTest(_diskId);
    }
    
    void MetadataBlockInfoTests::SharedLCMBInfoTest()
    {
        ::SharedLCMBInfoTest(_diskId,
                          1,              // Loops
                          0x800);         // Number entries
    }

    void MetadataBlockInfoTests::DedicatedLCMBInfoTest()
    {
        ::DedicatedLCMBInfoTest(_diskId,
                                1);              // Loops
    }
    
    bool MetadataBlockInfoTests::Setup()
    {
        ::SetupMBInfoTests(_diskId,
                           _startingAllocs,
                           _system);
                            
        return true;
    }

    bool MetadataBlockInfoTests::Cleanup()
    {
        ::CleanupMBInfoTests(_diskId,
                           _startingAllocs,
                             _system);
        
        return true;
    }

#endif  // UDRIVER || UPASSTHROUGH
#endif  // FEATURE_TEST

    //
    // Kernel mode only
    //
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
#if defined(UDRIVER) || defined(UPASSTHROUGH)
        // User mode only
        TEST_METHOD(CreateLogContainerWithBadPathTest);
#endif
#if defined(UPASSTHROUGH)
        TEST_METHOD(OpenCloseLogManagerStressTest);
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

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    void RawKtlLoggerTests::CreateLogContainerWithBadPathTest()
    {
        ::CreateLogContainerWithBadPathTest(_driveLetter);

    }
#endif

#if defined(UPASSTHROUGH)
    void RawKtlLoggerTests::OpenCloseLogManagerStressTest()
    {
        ::OpenCloseLogManagerStressTest();

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

#if defined(UDRIVER) || (UPASSTHROUGH)
    //
    // RAW Core logger tests
    //
    class RawCoreLoggerTests : public WEX::TestClass<RawCoreLoggerTests>
    {
    public:
        TEST_CLASS(RawCoreLoggerTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(TruncateToBadLogLowLsnTest);
        TEST_METHOD(StreamCheckpointBoundaryTest);
        TEST_METHOD(QueryLogTypeTest);
        TEST_METHOD(DeleteRecordsTest);
        TEST_METHOD(StreamCheckpointAtEndOfLogTest);

    private:
        KGuid _diskId;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
        
    };

    void RawCoreLoggerTests::StreamCheckpointAtEndOfLogTest()
    {
        ::StreamCheckpointAtEndOfLogTest(_diskId);
    }

    void RawCoreLoggerTests::TruncateToBadLogLowLsnTest()
    {
        ::TruncateToBadLogLowLsnTest(_diskId);
                              
    }

    void RawCoreLoggerTests::StreamCheckpointBoundaryTest()
    {
        ::StreamCheckpointBoundaryTest(_diskId);
    }

    void RawCoreLoggerTests::QueryLogTypeTest()
    {
        ::QueryLogTypeTest(_diskId);
    }

    void RawCoreLoggerTests::DeleteRecordsTest()
    {
        ::DeleteRecordsTest(_diskId);
    }

    bool RawCoreLoggerTests::Setup()
    {
        ::SetupRawCoreLoggerTests(_diskId,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }

    bool RawCoreLoggerTests::Cleanup()
    {
        ::CleanupRawCoreLoggerTests(_diskId,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }
#endif // UDRIVER
    

#if defined(UDRIVER) || defined(UPASSTHROUGH)
    class OverlayLogTests : public WEX::TestClass<OverlayLogTests>
    {
    public:
        TEST_CLASS(OverlayLogTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(ReadTest);
        TEST_METHOD(DestagedWriteTest);
        TEST_METHOD(CoalescedWriteTest);
        TEST_METHOD(CoalescedWrite2Test);
        TEST_METHOD(RecoveryTest);
        TEST_METHOD(Recovery2Test);     
        TEST_METHOD(RetryOnSharedLogFullTest);
        TEST_METHOD(RecoveryViaOpenContainerTest);
        TEST_METHOD(RecoveryPartlyCreatedStreamTest);
        TEST_METHOD(StreamQuotaTest);
        TEST_METHOD(DeleteTruncatedTailRecords1Test);
        TEST_METHOD(ThrottledAllocatorTest);

        //
        // Disable these for now as the implementation of
        // MultiRecordRead has changed and these scenarios will not be
        // hit. At some point we may want to delete them, but not yet.
        //
#if 0
        TEST_METHOD(MultiRecordCornerCaseTest);
        TEST_METHOD(MultiRecordCornerCase2Test);
        TEST_METHOD(ReadRaceConditionsTest);
#endif
        TEST_METHOD(OfflineDestageContainerTest);
        TEST_METHOD(WriteStuckConditionsTest);
        
        TEST_METHOD(VerifySharedTruncateOnDedicatedFailureTest);
        TEST_METHOD(FlushAllRecordsForCloseWaitTest);
        TEST_METHOD(WriteThrottleUnitTest);
        TEST_METHOD(ReadFromCoalesceBuffersTest);
        TEST_METHOD(ReadFromCoalesceBuffersCornerCase1Test);
        TEST_METHOD(ReadFromCoalesceBuffersTruncateTailTest);
        TEST_METHOD(ReadWriteRaceTest);
        TEST_METHOD(PeriodicTimerCloseRaceTest);
        
    private:
        KGuid _diskId;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
        
    };

#if 0 // TODO: Race conditions when reading from Coalesce and Dedicated
    void OverlayLogTests::ReadRaceConditionsTest()
    {
        ::ReadRaceConditionsTest(_diskId);                              
    }
#endif
    
    void OverlayLogTests::ReadWriteRaceTest()
    {
        ::ReadWriteRaceTest(_diskId);                              
    }
    
    void OverlayLogTests::WriteThrottleUnitTest()
    {
        ::WriteThrottleUnitTest(_diskId);                              
    }

    void OverlayLogTests::FlushAllRecordsForCloseWaitTest()
    {
        ::FlushAllRecordsForCloseWaitTest(_diskId);                              
    }
    
    void OverlayLogTests::VerifySharedTruncateOnDedicatedFailureTest()
    {
        ::VerifySharedTruncateOnDedicatedFailureTest(_diskId);                              
    }
    
    void OverlayLogTests::ReadFromCoalesceBuffersTest()
    {
        ::ReadFromCoalesceBuffersTest(_diskId);                              
    }

    void OverlayLogTests::ReadFromCoalesceBuffersCornerCase1Test()
    {
        ::ReadFromCoalesceBuffersCornerCase1Test(_diskId);                              
    }

    void OverlayLogTests::ReadFromCoalesceBuffersTruncateTailTest()
    {
        ::ReadFromCoalesceBuffersTruncateTailTest(_diskId);                              
    }
    
    void OverlayLogTests::PeriodicTimerCloseRaceTest()
    {
        ::PeriodicTimerCloseRaceTest(_diskId);                              
    }

    void OverlayLogTests::WriteStuckConditionsTest()
    {
        ::WriteStuckConditionsTest(_diskId);                              
    }
    
    void OverlayLogTests::ReadTest()
    {
        ::ReadTest(_diskId);                          
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
        ::Recovery2Test(_diskId);
                              
    }

#if 0
    void OverlayLogTests::MultiRecordCornerCaseTest()
    {
        ::MultiRecordCornerCaseTest(_diskId);                              
    }
    
    void OverlayLogTests::MultiRecordCornerCase2Test()
    {
        ::MultiRecordCornerCase2Test(_diskId);                              
    }

#endif
    
    void OverlayLogTests::OfflineDestageContainerTest()
    {
        ::OfflineDestageContainerTest(_diskId);                              
    }

    void OverlayLogTests::ThrottledAllocatorTest()
    {
        ::ThrottledAllocatorTest(_diskId);                              
    }

    void OverlayLogTests::RetryOnSharedLogFullTest()
    {
        ::RetryOnSharedLogFullTest(_diskId);                              
    }
    
    void OverlayLogTests::RecoveryViaOpenContainerTest()
    {
        ::RecoveryViaOpenContainerTest(_diskId);
                              
    }   
    void OverlayLogTests::RecoveryPartlyCreatedStreamTest()
    {
        ::RecoveryPartlyCreatedStreamTest(_diskId);
                              
    }   
    
    void OverlayLogTests::StreamQuotaTest()
    {
        ::StreamQuotaTest(_diskId);                              
    }   
    
    void OverlayLogTests::DeleteTruncatedTailRecords1Test()
    {
        ::DeleteTruncatedTailRecords1Test(_diskId);                              
    }   
    
    bool OverlayLogTests::Setup()
    {
        ::SetupOverlayLogTests(_diskId,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }

    bool OverlayLogTests::Cleanup()
    {
        ::CleanupOverlayLogTests(_diskId,
                                 _startingAllocs,
                                 _system);
                            
        return true;
    }   
#endif // UDRIVER || UPASSTHROUGH


#if defined(UDRIVER) || defined(UPASSTHROUGH)
    class KtlPhysicalLogTest : public WEX::TestClass<KtlPhysicalLogTest>
    {
    public:
        TEST_CLASS(KtlPhysicalLogTest)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(BasicDiskLoggerTest)
        TEST_METHOD(AliasTest)               
        TEST_METHOD(StreamTest)
#ifdef FEATURE_TEST
        TEST_METHOD(RecoveryTest)
#endif
        TEST_METHOD(StructVerify)
        TEST_METHOD(ReservationsTest)               
                
    private:
        WCHAR _driveLetter[2];
    };

    bool KtlPhysicalLogTest::Setup()
    {
        WCHAR systemDrive[32];
        ULONG result = ExpandEnvironmentStringsW(L"%SYSTEMDRIVE%", (LPWSTR)systemDrive, 32);
        VERIFY_ARE_NOT_EQUAL((ULONG)0, result, L"Failed to get systemdrive");
        VERIFY_IS_TRUE(result <= 32, L"Failed to get systemdrive");
        
        _driveLetter[0] = systemDrive[0];
        _driveLetter[1] = 0;

        return true;
    }

    bool KtlPhysicalLogTest::Cleanup()
    {
        return true;
    }

    void KtlPhysicalLogTest::BasicDiskLoggerTest()
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::BasicDiskLoggerTest(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"KTL Logger Testing must be run at elevated privs");
    }
    
    void KtlPhysicalLogTest::AliasTest()
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::RvdLoggerAliasTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status), L"KTL Logger Testing must be run at elevated privs");
    }
    
#ifdef FEATURE_TEST
    void KtlPhysicalLogTest::RecoveryTest()
    {
        NTSTATUS status;
        PWCHAR driveLetter = _driveLetter;

        status = ::RvdLoggerRecoveryTests(1, &driveLetter);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
    }
#endif
    
    void KtlPhysicalLogTest::StructVerify()
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


    void KtlPhysicalLogTest::StreamTest()
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
    
    void KtlPhysicalLogTest::ReservationsTest()
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
#endif  

#ifdef UDRIVER
    //
    // NOTE: This MUST be the last test in the list as the test does not cleanup properly, by design
    //
    class KtlLoggerMiscTests : public WEX::TestClass<KtlLoggerMiscTests>
    {
    public:
        TEST_CLASS(KtlLoggerMiscTests)

            TEST_CLASS_SETUP(Setup)
            TEST_CLASS_CLEANUP(Cleanup)

            TEST_METHOD(ForceCloseTest);
            TEST_METHOD(ForceCloseOnDeleteContainerWaitTest);

    private:
        KGuid _diskId;
        UCHAR _driveLetter;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
    };

    void KtlLoggerMiscTests::ForceCloseTest()
    {
        ::ForceCloseTest(_diskId);
    }

    void KtlLoggerMiscTests::ForceCloseOnDeleteContainerWaitTest()
    {
        ::ForceCloseOnDeleteContainerWaitTest(_diskId);
    }

    bool KtlLoggerMiscTests::Setup()
    {
        ::SetupMiscTests(_diskId,
            _driveLetter,
            _startingAllocs,
            _system);

        return true;
    }

    bool KtlLoggerMiscTests::Cleanup()
    {
        ::CleanupMiscTests(_diskId,
            _startingAllocs,
            _system);

        return true;
    }
#endif
}
