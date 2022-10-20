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
    //
    // Kernel mode only tests
    //
    class KtlLoggerTests : public WEX::TestClass<KtlLoggerTests>
    {
    public:
        TEST_CLASS(KtlLoggerTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(CorruptedLCMBInfoTest);
        TEST_METHOD(PartialContainerCreateTest);        
        TEST_METHOD(DeletedDedicatedLogTest)
        TEST_METHOD(StreamMetadataTest);

        TEST_METHOD(CloseOpenRaceTest);
        TEST_METHOD(OpenRightAfterCloseTest);

        TEST_METHOD(QueryRecordsTest)
#if defined(UDRIVER)        
        TEST_METHOD(FailCoalesceFlushTest)
#endif                
        TEST_METHOD(ManyKIoBufferElementsTest)
        TEST_METHOD(CloseTest)
        TEST_METHOD(ThresholdTest)
        TEST_METHOD(CancelStressTest)
        TEST_METHOD(DLogReservationTest)
        TEST_METHOD(EnumContainersTest)
        TEST_METHOD(DeleteOpenStreamTest)
#if 0  // TODO: Reenable when this can be made more reliable
        TEST_METHOD(VerifySparseTruncateTest)
#endif
                                
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
    
    void KtlLoggerTests::CorruptedLCMBInfoTest()
    {
        ::CorruptedLCMBInfoTest(_driveLetter, _logManagers[0]);
    }
            
    void KtlLoggerTests::PartialContainerCreateTest()
    {
        ::PartialContainerCreateTest(_driveLetter, _logManagers[0]);
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

    void KtlLoggerTests::QueryRecordsTest()
    {
        ::QueryRecordsTest(_diskId, _logManagers[0]);
    }

#if defined(UDRIVER)
    void KtlLoggerTests::FailCoalesceFlushTest()
    {
        ::FailCoalesceFlushTest(_diskId, _logManagers[0]);
    }
#endif
    
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

    void KtlLoggerTests::CloseTest()
    {
        ::CloseTest(_diskId, _logManagers[0]);
    }

    void KtlLoggerTests::DeleteOpenStreamTest()
    {
        ::DeleteOpenStreamTest(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::DLogReservationTest()
    {
        ::DLogReservationTest(_diskId, _logManagers[0]);
    }
        
    void KtlLoggerTests::DeletedDedicatedLogTest()
    {
        ::DeletedDedicatedLogTest(_diskId, _logManagers[0]);
    }
    
#if 0  // TODO: Reenable when this can be made more reliable
    void KtlLoggerTests::VerifySparseTruncateTest()
    {
        ::VerifySparseTruncateTest(_driveLetter, _logManagers[0]);
    }
#endif

    void KtlLoggerTests::EnumContainersTest()
    {
        ::EnumContainersTest(_diskId, _logManagers[0]);
    }

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
        TEST_METHOD(DuplicateRecordInLogTest);
        TEST_METHOD(CorruptedRecordTest);
		
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

    void RawCoreLoggerTests::DuplicateRecordInLogTest()
    {
        ::DuplicateRecordInLogTest(_diskId);
    }

    void RawCoreLoggerTests::CorruptedRecordTest()
    {
        ::CorruptedRecordTest(_diskId);
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

        TEST_METHOD(RetryOnSharedLogFullTest);
        TEST_METHOD(RecoveryPartlyCreatedStreamTest);
        TEST_METHOD(StreamQuotaTest);
        TEST_METHOD(WriteThrottleUnitTest);
        TEST_METHOD(ThrottledAllocatorTest);
        TEST_METHOD(WriteStuckConditionsTest);        
        TEST_METHOD(VerifySharedTruncateOnDedicatedFailureTest);
		TEST_METHOD(VerifyCopyFromSharedToBackupTest)
        TEST_METHOD(FlushAllRecordsForCloseWaitTest);
        TEST_METHOD(ReadFromCoalesceBuffersCornerCase1Test);
        TEST_METHOD(ReadFromCoalesceBuffersTruncateTailTest);
        TEST_METHOD(ReadWriteRaceTest);
        TEST_METHOD(PeriodicTimerCloseRaceTest);
        TEST_METHOD(DiscontiguousRecordsRecoveryTest);
		
    private:
        KGuid _diskId;
        UCHAR _driveLetter;
        ULONGLONG _startingAllocs;
        KtlSystem* _system;
        
    };

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
	
    void OverlayLogTests::VerifyCopyFromSharedToBackupTest()
    {
        ::VerifyCopyFromSharedToBackupTest(_driveLetter, _diskId);                              
    }
    
    void OverlayLogTests::VerifySharedTruncateOnDedicatedFailureTest()
    {
        ::VerifySharedTruncateOnDedicatedFailureTest(_diskId);                              
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

    void OverlayLogTests::DiscontiguousRecordsRecoveryTest()
    {
        ::DiscontiguousRecordsRecoveryTest(_diskId);                              
    }
	
    void OverlayLogTests::WriteStuckConditionsTest()
    {
        ::WriteStuckConditionsTest(_diskId);                              
    }
    
    void OverlayLogTests::ThrottledAllocatorTest()
    {
        ::ThrottledAllocatorTest(_diskId);                              
    }

    void OverlayLogTests::RetryOnSharedLogFullTest()
    {
        ::RetryOnSharedLogFullTest(_diskId);                              
    }
    
    void OverlayLogTests::RecoveryPartlyCreatedStreamTest()
    {
        ::RecoveryPartlyCreatedStreamTest(_diskId);
                              
    }   
    
    void OverlayLogTests::StreamQuotaTest()
    {
        ::StreamQuotaTest(_diskId);                              
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
