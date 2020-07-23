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
    class KtlLoggerTests : public WEX::TestClass<KtlLoggerTests>
    {
    public:
        TEST_CLASS(KtlLoggerTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)
                
        TEST_METHOD(ContainerLimitsTest);
        TEST_METHOD(Stress1Test)
        TEST_METHOD(AliasStress);
        TEST_METHOD(LLDataWorkloadTest)
        TEST_METHOD(AccelerateFlushTest)                


#if 0
// Disable for now - re-enable after shipping 2.2               
//        TEST_METHOD(ServiceWrapperStressTest)
#endif  // FEATURE_TEST
                
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
    
    void KtlLoggerTests::AccelerateFlushTest()
    {
        ::AccelerateFlushTest(_diskId, _logManagers[0]);
    }    

    void KtlLoggerTests::ContainerLimitsTest()
    {
        ::ContainerLimitsTest(_diskId, _logManagers[0]);
    }    

    void KtlLoggerTests::AliasStress()
    {
        ::AliasStress(_diskId, _logManagers[0]);
    }
    
    void KtlLoggerTests::Stress1Test()
    {
        ::Stress1Test(_diskId, _LogManagerCount, _logManagers);
    }
    
    void KtlLoggerTests::LLDataWorkloadTest()
    {
        ::LLDataWorkloadTest(_diskId, _logManagers[0]);
    }

#if 0   // Reenable later after shipping 2.2
    void KtlLoggerTests::ServiceWrapperStressTest()
    {
        ::ServiceWrapperStressTest(_diskId, _logManagers[0]);
    }
#endif // 0 

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

}
