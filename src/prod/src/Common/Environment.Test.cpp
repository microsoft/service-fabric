// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#if !defined(PLATFORM_UNIX)
#include "fabriccommon_.h"
#include "fabricruntime_.h"
#include "fabricclient_.h"
#endif

namespace Common
{
    #define EnvironmentTraceType "EnvironmentTest"

    using namespace std;

    class EnvironmentTest : protected TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    };

    BOOST_FIXTURE_TEST_SUITE(EnvironmentTestSuite,EnvironmentTest)

    BOOST_AUTO_TEST_CASE(GetEnviromentMapTest)
    {
#if defined(PLATFORM_UNIX)
        VERIFY_ARE_EQUAL(setenv("_TestEnvVariable1_", "_TestEnvValue1", 1), 0);
#else
        VERIFY_ARE_NOT_EQUAL(::SetEnvironmentVariableW(L"_TestEnvVariable1_", L"_TestEnvValue1"), 0);
#endif

        EnvironmentMap envMap;
        VERIFY_IS_TRUE(Environment::GetEnvironmentMap(envMap), L"GetEnvironmentMap");
        VERIFY_ARE_EQUAL2(envMap[L"_TestEnvVariable1_"], L"_TestEnvValue1");
    }
    
    BOOST_AUTO_TEST_CASE(CopyAssignmentEnvironmentTest)
    {
        EnvironmentVariable ev(L"TestKey1", L"TestVal1");
        ev = L"TestVal2";
        EnvironmentMap envMap1;
        VERIFY_IS_TRUE(Environment::GetEnvironmentMap(envMap1), L"GetEnvironmentMap");
        VERIFY_ARE_EQUAL2(envMap1[L"TestKey1"], L"TestVal2");
        ev = L"TestVal1";
        EnvironmentMap envMap2;
        VERIFY_IS_TRUE(Environment::GetEnvironmentMap(envMap2), L"GetEnvironmentMap");
        VERIFY_ARE_EQUAL2(envMap2[L"TestKey1"], L"TestVal1");
    }

#if defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(ProcessorCountTest)
    {
        std::shared_ptr<FILE> pipe(popen("grep -w \"core id\" /proc/cpuinfo | wc -l", "r"), pclose);
        char buffer[128];
        std::string result = "";
        if (fgets(buffer, 128, pipe.get()) != NULL)
        {
            result += buffer;
        }
        VERIFY_ARE_EQUAL(Environment::GetNumberOfProcessors(), atoi(result.c_str()));
    }
#endif
    
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(ToEnvironmentBlockTest)
    {
        EnvironmentMap envMap;
        VERIFY_IS_TRUE(Environment::GetEnvironmentMap(envMap), L"GetEnvironmentMap");

        envMap[L"_TestEnvVariable2_"] = L"_TestEnvValue2";

        vector<wchar_t> envBlock;
        Environment::ToEnvironmentBlock(envMap, envBlock);
        
        VERIFY_ARE_NOT_EQUAL(::SetEnvironmentStringsW(&(envBlock.front())), 0);

        vector<wchar_t> buffer;
        buffer.resize(36);

        DWORD result = ::GetEnvironmentVariableW(L"_TestEnvVariable2_", &buffer.front(), 36);
        VERIFY_ARE_NOT_EQUAL(result, (DWORD)0);

        wstring testEnvValue2(&buffer.front());

        VERIFY_ARE_EQUAL2(testEnvValue2, L"_TestEnvValue2");
    }

    BOOST_AUTO_TEST_CASE(FileVersionTest)
    {
        ComPointer<IFabricStringResult> commonDllVersion;
        auto hr  = ::FabricGetCommonDllVersion(commonDllVersion.InitializationAddress());
        
        VERIFY_ARE_EQUAL2(SUCCEEDED(hr), true);
        WriteInfo(EnvironmentTraceType, "CommonDllVersion: {0}", commonDllVersion->get_String());

         
        ComPointer<IFabricStringResult> runtimeDllVersion;
        hr  = ::FabricGetRuntimeDllVersion(runtimeDllVersion.InitializationAddress());
         
        VERIFY_ARE_EQUAL2(SUCCEEDED(hr), true);
        WriteInfo(EnvironmentTraceType, "RuntimeDllVersion: {0}", runtimeDllVersion->get_String());

        ComPointer<IFabricStringResult> clientDllVersion;
        hr  = ::FabricGetClientDllVersion(clientDllVersion.InitializationAddress());
         
        VERIFY_ARE_EQUAL2(SUCCEEDED(hr), true);
        WriteInfo(EnvironmentTraceType, "ClientDllVersion: {0}", clientDllVersion->get_String());
     }

     BOOST_AUTO_TEST_CASE(LastRebootTimeTest)
     {
         ErrorCode error(ErrorCodeValue::Success);
         DateTime nowTime = DateTime::Now();
         DateTime lastRebootTime = Environment::GetLastRebootTime();
         VERIFY_IS_TRUE(nowTime > lastRebootTime);

         wstring pathToOldFile;
         DateTime oldFileWriteTime;
         Environment::GetEnvironmentVariableW(L"SystemRoot", pathToOldFile);
         Path::CombineInPlace(pathToOldFile, L"System32\\rundll32.exe");
         error = File::GetLastWriteTime(pathToOldFile, oldFileWriteTime);
         VERIFY_IS_TRUE(error.IsSuccess());
         VERIFY_IS_TRUE(oldFileWriteTime < lastRebootTime);
     }
#endif

     BOOST_AUTO_TEST_SUITE_END()
}
