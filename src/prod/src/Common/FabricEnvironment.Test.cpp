// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Expression.h"
#include "Operators.h"

using namespace std;

namespace Common
{
    class FabricEnvironmentTest
    {
    protected:
            FabricEnvironmentTest() { BOOST_REQUIRE(Setup()); }
            TEST_CLASS_SETUP(Setup)
            ~FabricEnvironmentTest() { BOOST_REQUIRE(Cleanup()); }
            TEST_CLASS_CLEANUP(Cleanup)

        void VerifyRegistryKeyExistence(bool shouldExist);
        bool registryKeyExists_;
        wstring fabricDataRoot_;
        wstring fabricLogRoot_;
        wstring fabricBinRoot_;
        wstring fabricCodePath_;
    };



    BOOST_FIXTURE_TEST_SUITE(FabricEnvironmentTestSuite,FabricEnvironmentTest)

    BOOST_AUTO_TEST_CASE(NoEnvironmentSet)
    {
        wstring value;
        ErrorCode error = FabricEnvironment::GetFabricBinRoot(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricBinRootNotFound));
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricCodePath(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricCodePathNotFound));
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricDataRoot(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricDataRootNotFound));
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricLogRoot(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricLogRootNotFound));
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricDnsServerIPAddress(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::DnsServerIPAddressNotFound));
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricHostServicePath(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricHostServicePathNotFound));
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetUpdaterServicePath(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::UpdaterServicePathNotFound));
        VerifyRegistryKeyExistence(false);
    }
    
    BOOST_AUTO_TEST_CASE(EnvironmentVariablesSet)
    {
        const wstring MockDnsServerIPAddress = L"0.1.2.3";

        Environment::SetEnvironmentVariableW(L"FabricDataRoot", L"FabricDataRoot");
        Environment::SetEnvironmentVariableW(L"FabricBinRoot", L"FabricBinRoot");
        Environment::SetEnvironmentVariableW(L"FabricCodePath", L"FabricCodePath");
        Environment::SetEnvironmentVariableW(L"FabricLogRoot", L"FabricLogRoot");
        Environment::SetEnvironmentVariableW(L"FabricLogRoot", L"FabricLogRoot");
        Environment::SetEnvironmentVariableW(L"FabricDnsServerIPAddress", MockDnsServerIPAddress);
        Environment::SetEnvironmentVariableW(L"FabricHostServicePath", L"FabricHostServicePath");
        Environment::SetEnvironmentVariableW(L"UpdaterServicePath", L"UpdaterServicePath");

        wstring value;
        ErrorCode error = FabricEnvironment::GetFabricBinRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricBinRoot");
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricCodePath(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricCodePath");
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricDataRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricDataRoot");
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricLogRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricLogRoot");
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricDnsServerIPAddress(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, MockDnsServerIPAddress);
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetFabricHostServicePath(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricHostServicePath");
        VerifyRegistryKeyExistence(false);

        error = FabricEnvironment::GetUpdaterServicePath(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"UpdaterServicePath");
        VerifyRegistryKeyExistence(false);
    }

    BOOST_AUTO_TEST_CASE(EnvironmentVariablesAndRegistryKeySet)
    {
        const wstring MockDnsServerIPAddressEnvtVal = L"0.1.2.3";
        const wstring MockDnsServerIPAddressRegVal = L"4.5.6.7";

        Environment::SetEnvironmentVariableW(L"FabricDataRoot", L"FabricDataRoot");
        Environment::SetEnvironmentVariableW(L"FabricBinRoot", L"FabricBinRoot");
        Environment::SetEnvironmentVariableW(L"FabricCodePath", L"FabricCodePath");
        Environment::SetEnvironmentVariableW(L"FabricLogRoot", L"FabricLogRoot");
        Environment::SetEnvironmentVariableW(L"EnableCircularTraceSession", L"1");
        Environment::SetEnvironmentVariableW(L"FabricDnsServerIPAddress", MockDnsServerIPAddressEnvtVal);
        Environment::SetEnvironmentVariableW(L"FabricHostServicePath", L"FabricHostServicePath");
        Environment::SetEnvironmentVariableW(L"UpdaterServicePath", L"UpdaterServicePath");

        // Create regkey
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, false, false);
        regKey.SetValue(FabricConstants::FabricDataRootRegKeyName, L"FabricDataRoot");
        regKey.SetValue(FabricConstants::FabricBinRootRegKeyName, L"FabricBinRoot");
        regKey.SetValue(FabricConstants::FabricCodePathRegKeyName, L"FabricCodePath");
        regKey.SetValue(FabricConstants::FabricLogRootRegKeyName, L"FabricLogRoot");
        regKey.SetValue(FabricConstants::EnableCircularTraceSessionRegKeyName, 1);
        regKey.SetValue(FabricConstants::FabricDnsServerIPAddressRegKeyName, MockDnsServerIPAddressRegVal);
        regKey.SetValue(FabricConstants::FabricHostServicePathRegKeyName, L"FabricHostServicePath");
        regKey.SetValue(FabricConstants::UpdaterServicePathRegKeyName, L"UpdaterServicePath");

        wstring value;
        ErrorCode error = FabricEnvironment::GetFabricBinRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricBinRoot");
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricCodePath(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricCodePath");
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricDataRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricDataRoot");
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricLogRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricLogRoot");
        VerifyRegistryKeyExistence(true);

        BOOLEAN bValue;
        error = FabricEnvironment::GetEnableCircularTraceSession(bValue);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE_FMT(bValue == TRUE, "Env value 1 should be treated as true.");
        VerifyRegistryKeyExistence(true);

        // Verify disabled case
        Environment::SetEnvironmentVariableW(L"EnableCircularTraceSession", L"0");
        error = FabricEnvironment::GetEnableCircularTraceSession(bValue);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE_FMT(bValue == FALSE, "Env value 0 should be treated as false.");
        VerifyRegistryKeyExistence(true);

        // Verify envt. var takes precedence over reg key
        error = FabricEnvironment::GetFabricDnsServerIPAddress(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, MockDnsServerIPAddressEnvtVal);
        VerifyRegistryKeyExistence(true);

        // Remove env var and test with regkey only
        Environment::SetEnvironmentVariableW(L"EnableCircularTraceSession", L"");
        regKey.SetValue(FabricConstants::EnableCircularTraceSessionRegKeyName, 1);
        error = FabricEnvironment::GetEnableCircularTraceSession(bValue);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE_FMT(bValue == TRUE, "Registry 1 should be treated as true.");
        VerifyRegistryKeyExistence(true);

        Environment::SetEnvironmentVariableW(L"FabricDnsServerIPAddress", L"");
        error = FabricEnvironment::GetFabricDnsServerIPAddress(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, MockDnsServerIPAddressRegVal);
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricHostServicePath(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"FabricHostServicePath");
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetUpdaterServicePath(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"UpdaterServicePath");
        VerifyRegistryKeyExistence(true);

        Environment::SetEnvironmentVariableW(L"FabricDataRoot", L"");
        Environment::SetEnvironmentVariableW(L"FabricBinRoot", L"");
        Environment::SetEnvironmentVariableW(L"FabricCodePath", L"");
        Environment::SetEnvironmentVariableW(L"FabricLogRoot", L"");
        Environment::SetEnvironmentVariableW(L"EnableCircularTraceSession", L"1");
        Environment::SetEnvironmentVariableW(L"FabricHostServicePath", L"");
        Environment::SetEnvironmentVariableW(L"UpdaterServicePath", L"");
    }

    BOOST_AUTO_TEST_CASE(OnlyRegistryRootSet)
    {        
        // Create regkey
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, false, false);
        regKey.SetValue(FabricConstants::EnableCircularTraceSessionRegKeyName, 1);

        wstring value;
        ErrorCode error = FabricEnvironment::GetFabricBinRoot(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricBinRootNotFound));
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricCodePath(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricCodePathNotFound));
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricDataRoot(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricDataRootNotFound));
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricLogRoot(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricLogRootNotFound));
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricDnsServerIPAddress(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::DnsServerIPAddressNotFound));
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetFabricHostServicePath(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::FabricHostServicePathNotFound));
        VerifyRegistryKeyExistence(true);

        error = FabricEnvironment::GetUpdaterServicePath(value);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::UpdaterServicePathNotFound));
        VerifyRegistryKeyExistence(true);
    }

    BOOST_AUTO_TEST_CASE(RegistryKeySet)
    {
        VERIFY_IS_TRUE(true);
    }

    BOOST_AUTO_TEST_CASE(ExpandVariable)
    {
        Environment::SetEnvironmentVariableW(L"__TestEnv", L"TestEnv");
        Environment::SetEnvironmentVariableW(L"FabricDataRoot", L"%__TestEnv%\\FabricDataRoot");
        wstring value;
        ErrorCode error = FabricEnvironment::GetFabricDataRoot(value);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(value, L"TestEnv\\FabricDataRoot");
        VerifyRegistryKeyExistence(false);
        VERIFY_IS_TRUE(true);
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool FabricEnvironmentTest::Setup()
    {
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
        if (regKey.IsValid)
        {
            registryKeyExists_ = true;
            if (!regKey.GetValue(FabricConstants::FabricCodePathRegKeyName, this->fabricCodePath_))
            {
                this->fabricCodePath_ = L"";
            }
            else
            {
                regKey.DeleteValue(FabricConstants::FabricCodePathRegKeyName);
            }
            
            if (!regKey.GetValue(FabricConstants::FabricDataRootRegKeyName, this->fabricDataRoot_))
            {
                this->fabricDataRoot_ = L"";
            }
            else
            {
                regKey.DeleteValue(FabricConstants::FabricDataRootRegKeyName);
            }

            if (!regKey.GetValue(FabricConstants::FabricLogRootRegKeyName, this->fabricLogRoot_))
            {
                this->fabricLogRoot_ = L"";
            }
            else
            {
                regKey.DeleteValue(FabricConstants::FabricLogRootRegKeyName);
            }

            if (!regKey.GetValue(FabricConstants::FabricBinRootRegKeyName, this->fabricBinRoot_))
            {
                this->fabricBinRoot_ = L"";
            }
            else
            {
                regKey.DeleteValue(FabricConstants::FabricBinRootRegKeyName);
            }
            return regKey.DeleteKey();
        }
        return true;
    }

    bool FabricEnvironmentTest::Cleanup()
    {
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
        if (regKey.IsValid)
        {
            if (registryKeyExists_ == true)
            {
                if (this->fabricCodePath_ != L"")
                {
                    regKey.SetValue(FabricConstants::FabricCodePathRegKeyName, this->fabricCodePath_);
                }
                if (this->fabricDataRoot_ != L"")
                {
                    regKey.SetValue(FabricConstants::FabricDataRootRegKeyName, this->fabricDataRoot_);
                }
                if (this->fabricLogRoot_ != L"")
                {
                    regKey.SetValue(FabricConstants::FabricLogRootRegKeyName, this->fabricLogRoot_);
                }
                if (this->fabricBinRoot_ != L"")
                {
                    regKey.SetValue(FabricConstants::FabricBinRootRegKeyName, this->fabricBinRoot_);
                }
            }
            else
            {
                regKey.DeleteKey();
            }
        }
        return true;
    }


    void FabricEnvironmentTest::VerifyRegistryKeyExistence(bool shouldExist)
    {
        RegistryKey regKey(FabricConstants::FabricRegistryKeyPath, true, true);
        VERIFY_ARE_EQUAL2(shouldExist, regKey.IsValid);
    }
}

