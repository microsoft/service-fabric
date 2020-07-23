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
#include "LWTTests.h"

#include "RWTStress.h"

#define ALLOCATION_TAG 'LLT'


namespace KtlPhysicalLogTest
{
    //
    // Kernel mode only tests
    //
    class LWTTests : public WEX::TestClass<LWTTests>
    {
    public:
        TEST_CLASS(LWTTests)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(VersionTest);
		TEST_METHOD(LongRunningWriteTest);
                
    private:
        NTSTATUS LWTTests::FindDiskIdForDriveLetter(
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

    void LWTTests::VersionTest()
    {
        ::VersionTest(_diskId, _logManagers[0]);
    }

    void LWTTests::LongRunningWriteTest()
    {
        ::LongRunningWriteTest(_diskId, _logManagers[0]);
    }

    NTSTATUS LWTTests::FindDiskIdForDriveLetter(
        UCHAR& DriveLetter,
        GUID& DiskIdGuid
        )
    {
        return(::FindDiskIdForDriveLetter(DriveLetter, DiskIdGuid));
    }
    
    bool LWTTests::Setup()
    {
        ::SetupTests(_diskId,
                     _driveLetter,
                     _LogManagerCount,
                     _logManagers,
                     _startingAllocs,
                     _system);
                            
        return true;
    }

    bool LWTTests::Cleanup()
    {
        ::CleanupTests(_diskId,
                     _LogManagerCount,
                     _logManagers,
                     _startingAllocs,
                     _system);
        
        return true;
    }
}
