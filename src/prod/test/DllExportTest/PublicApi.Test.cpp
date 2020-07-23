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
    #define PublicApiTraceType "PublicApiTest"

    using namespace std;

    class PublicApiTest : protected TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    };

    BOOST_FIXTURE_TEST_SUITE(PublicApiTestSuite,PublicApiTest)

#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(FileVersionTest)
    {
        ComPointer<IFabricStringResult> commonDllVersion;
        auto hr  = ::FabricGetCommonDllVersion(commonDllVersion.InitializationAddress());
        
        VERIFY_ARE_EQUAL2(SUCCEEDED(hr), true);
        WriteInfo(PublicApiTraceType, "CommonDllVersion: {0}", commonDllVersion->get_String());

         
        ComPointer<IFabricStringResult> runtimeDllVersion;
        hr  = ::FabricGetRuntimeDllVersion(runtimeDllVersion.InitializationAddress());
         
        VERIFY_ARE_EQUAL2(SUCCEEDED(hr), true);
        WriteInfo(PublicApiTraceType, "RuntimeDllVersion: {0}", runtimeDllVersion->get_String());

        ComPointer<IFabricStringResult> clientDllVersion;
        hr  = ::FabricGetClientDllVersion(clientDllVersion.InitializationAddress());
         
        VERIFY_ARE_EQUAL2(SUCCEEDED(hr), true);
        WriteInfo(PublicApiTraceType, "ClientDllVersion: {0}", clientDllVersion->get_String());
     }

        //LINUXTODO enable this test when setting algorithm is supported on Linux
    BOOST_AUTO_TEST_CASE(EncryptTest_SetAlgorithm)
    {
        InstallTestCertInScope testCert;

        ComPointer<IFabricStringResult> encryptedResult;
        wstring text = L"The lazy fox jumped";
        auto hr = ::FabricEncryptText(
            text.c_str(),
            testCert.Thumbprint()->ToStrings().first.c_str(),
            X509Default::StoreName().c_str(),
            X509Default::StoreLocation_Public(),
            szOID_NIST_AES192_CBC, //LINUXTODO, need to define this?
            encryptedResult.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);

        ComPointer<IFabricStringResult> decryptedResult;
        hr = ::FabricDecryptText(
            encryptedResult->get_String(),
            X509Default::StoreLocation_Public(),
            decryptedResult.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);
        VERIFY_ARE_EQUAL2(decryptedResult->get_String(), text);
    }

    BOOST_AUTO_TEST_CASE(EncryptTest_CertFile)
    {
        const wstring certFileName = L"WinFabric-Test-SAN1-Alice.cer";
        wstring bins;
        if (!Environment::GetEnvironmentVariableW(L"_NTTREE", bins, Common::NOTHROW()))
        {
            bins = Directory::GetCurrentDirectoryW();
        }

        if (!File::Exists(certFileName))
        {
            wstring certFileInBins = Path::Combine(bins, L"FabricUnitTests\\" + certFileName);
            // copy the binplaced certificate file
            VERIFY_IS_TRUE(File::Copy(certFileInBins, certFileName).IsSuccess());
        }

        ComPointer<IFabricStringResult> encryptedResult;
        wstring text = L"The lazy fox jumped";
        auto hr = ::FabricEncryptText2(
            text.c_str(),
            L"WinFabric-Test-SAN1-Alice.cer",
            nullptr,
            encryptedResult.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);

        ComPointer<IFabricStringResult> decryptedResult;
        hr = ::FabricDecryptText(
            encryptedResult->get_String(),
            X509Default::StoreLocation_Public(),
            decryptedResult.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);
        VERIFY_ARE_EQUAL2(decryptedResult->get_String(), text);
    }

#endif

    BOOST_AUTO_TEST_CASE(EncryptTest) // Test with public API
    {
        InstallTestCertInScope testCert;

        ComPointer<IFabricStringResult> encryptedResult;
        wstring text = L"The lazy fox jumped";
        auto hr = ::FabricEncryptText(
            text.c_str(),
            testCert.Thumbprint()->ToStrings().first.c_str(),
            X509Default::StoreName().c_str(),
            X509Default::StoreLocation_Public(),
            nullptr,
            encryptedResult.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);

        ComPointer<IFabricStringResult> encryptedResult2;
        hr = ::FabricEncryptText(
            text.c_str(),
            testCert.Thumbprint()->ToStrings().first.c_str(),
            X509Default::StoreName().c_str(),
            X509Default::StoreLocation_Public(),
            nullptr,
            encryptedResult2.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);

        // two identical encrypt calls should produce different results
        VERIFY_IS_TRUE(encryptedResult->get_String() != encryptedResult2->get_String());

        ComPointer<IFabricStringResult> decryptedResult;
        hr = ::FabricDecryptText(
            encryptedResult->get_String(),
            X509Default::StoreLocation_Public(),
            decryptedResult.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);
        VERIFY_ARE_EQUAL2(decryptedResult->get_String(), text);

        ComPointer<IFabricStringResult> decryptedResult2;
        hr = ::FabricDecryptText(
            encryptedResult2->get_String(),
            X509Default::StoreLocation_Public(),
            decryptedResult2.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, 0);
        VERIFY_ARE_EQUAL2(decryptedResult2->get_String(), text);

        ComPointer<IFabricStringResult> encryptedResult3;

        // The max length is specified by MaxLongStringSize (1024) in DllExportTest.exe.cfg.
        // The value of MaxLongStringSize controls TRY_COM_PARSE_PUBLIC_LONG_STRING marco.
        text = wstring(2048, 't');
        hr = ::FabricEncryptText(
            text.c_str(),
            testCert.Thumbprint()->ToStrings().first.c_str(),
            X509Default::StoreName().c_str(),
            X509Default::StoreLocation_Public(),
            nullptr,
            encryptedResult3.InitializationAddress());

        VERIFY_ARE_EQUAL2(hr, STRSAFE_E_INVALID_PARAMETER);
    }


     BOOST_AUTO_TEST_SUITE_END()
}
