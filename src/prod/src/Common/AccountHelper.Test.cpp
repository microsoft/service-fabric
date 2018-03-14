// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/CryptoTest.h"

using namespace std;
using namespace Common;

namespace Common
{
    class AccountHelperTest
    {
    public:
        static void TraceAccountNames(map<wstring, wstring> const & accountNames);

    protected:
        SecurityTestSetup securityTestSetup_;
    };

    BOOST_FIXTURE_TEST_SUITE2(AccountHelperSuite, AccountHelperTest);

    BOOST_AUTO_TEST_CASE(GeneratePwd)
    {
        const wstring accountName = L"redmond\testAccount";
        const SecureString testSecret(L"test secret");
        SecureString password;
        auto error = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            accountName,
            testSecret,
            password); 

        Trace.WriteInfo(TraceType, "errorCode={0}, password='{1}'", error, password);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(!password.IsEmpty());

        SecureString password2;
        error = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            accountName,
            testSecret,
            password2); 

        Trace.WriteInfo(TraceType, "errorCode={0}, password2='{1}'", error, password2);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(password, password2);
    }

    BOOST_AUTO_TEST_CASE(GeneratePwdFromX509)
    {
        InstallTestCertInScope testCert;
        
        const wstring accountName = L"redmond\testAccount";
        const SecureString testSecret(L"test secret");
        SecureString password;
        auto error = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            accountName,
            testSecret,
            X509Default::StoreLocation(),
            X509Default::StoreName(),
            testCert.Thumbprint()->ToStrings().first,
            password); 

        Trace.WriteInfo(TraceType, "errorCode={0}, password='{1}'", error, password);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(!password.IsEmpty());

        SecureString password2;
        error = AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
            accountName,
            testSecret,
            X509Default::StoreLocation(),
            X509Default::StoreName(),
            testCert.Thumbprint()->ToStrings().first,
            password2); 

        Trace.WriteInfo(TraceType, "errorCode={0}, password2='{1}'", error, password2);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL2(password, password2);
    }

    BOOST_AUTO_TEST_CASE(GetAccountNamesFromX509CommonName)
    {
        // Install a certificate with a specified common name
        wstring commonName = wformatString("GetCnAccountNamesTest{0}", Guid::NewGuid());
        wstring subjectName = L"CN=" + commonName;
        InstallTestCertInScope testCert(subjectName);

        wstring actualCommonName;
        auto error = CryptoUtility::GetCertificateCommonName(
            testCert.CertContext(),
            actualCommonName);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_ARE_EQUAL(testCert.SubjectName(), subjectName);
        VERIFY_ARE_EQUAL(commonName, actualCommonName);

        map<wstring, wstring> accountNames;
        error = AccountHelper::GetAccountNamesWithCertificateCommonName(
            commonName,
            testCert.StoreLocation(),
            testCert.StoreName(),
            true /*generateV1Account*/,
            accountNames);
        VERIFY_IS_TRUE(error.IsSuccess());

        TraceAccountNames(accountNames);
        
        // Expect one certificate for V1, one for V2, with the thumbprint equal to the testCert's thumbprint
        VERIFY_IS_TRUE(accountNames.size() == 2u);

        Thumbprint::SPtr thumbprint = testCert.Thumbprint();
        wstring expectedThumbprint = wformatString(thumbprint);

        for (auto const & accountName : accountNames)
        {
            wstring actualThumbprint = accountName.second;
            Trace.WriteInfo(TraceType, "GetAccountNamesFromX509CommonName: Verify expectedThumbprint {0} equals actual {1}", expectedThumbprint, actualThumbprint);
            VERIFY_ARE_EQUAL(expectedThumbprint, actualThumbprint);
        }

        //
        // Install one more certificate with the same common name
        //
        InstallTestCertInScope testCert2(subjectName);
        map<wstring, wstring> accountNames2;
        error = AccountHelper::GetAccountNamesWithCertificateCommonName(
            commonName,
            testCert2.StoreLocation(),
            testCert2.StoreName(),
            false,
            accountNames2);
        VERIFY_IS_TRUE(error.IsSuccess());

        TraceAccountNames(accountNames2);
        
        // V1 accounts are not generated, so expect 2 accounts using the new formats for the 2 certificates.
        VERIFY_IS_TRUE(accountNames2.size() == 2);

        wstring expectedThumbprint2 = wformatString(testCert2.Thumbprint());
        {
            auto it = find_if(accountNames2.begin(), accountNames2.end(),
                [expectedThumbprint](auto const & entry) { return expectedThumbprint == entry.second; });
            VERIFY_IS_TRUE(it != accountNames2.end());
            auto it2 = find_if(accountNames2.begin(), accountNames2.end(),
                [expectedThumbprint2](auto const & entry) { return expectedThumbprint2 == entry.second; });
            VERIFY_IS_TRUE(it2 != accountNames2.end());
        }

        //
        // Install a new certificate with expiration date higher than max supported of 2049. The expiration date is truncated.
        //
        InstallTestCertInScope testCert3(subjectName, nullptr, TimeSpan::FromHours(24 * 365 * 100) /*100 years*/);
        map<wstring, wstring> accountNames3;
        error = AccountHelper::GetAccountNamesWithCertificateCommonName(
            commonName,
            testCert3.StoreLocation(),
            testCert3.StoreName(),
            false /*generateV1Account*/,
            accountNames3);
        VERIFY_IS_TRUE(error.IsSuccess());

        TraceAccountNames(accountNames3);

        // V1 accounts are not generated, so expect 3 accounts using the new formats for the 3 certificates.
        VERIFY_IS_TRUE(accountNames3.size() == 3);

        wstring expectedThumbprint3 = wformatString(testCert3.Thumbprint());
        {
            auto it = find_if(accountNames3.begin(), accountNames3.end(),
                [expectedThumbprint](auto const & entry) { return expectedThumbprint == entry.second; });
            VERIFY_IS_TRUE(it != accountNames3.end());
            auto it2 = find_if(accountNames3.begin(), accountNames3.end(),
                [expectedThumbprint2](auto const & entry) { return expectedThumbprint2 == entry.second; });
            VERIFY_IS_TRUE(it2 != accountNames3.end());
            auto it3 = find_if(accountNames3.begin(), accountNames3.end(),
                [expectedThumbprint3](auto const & entry) { return expectedThumbprint3 == entry.second; });
            VERIFY_IS_TRUE(it3 != accountNames3.end());
        }

        testCert.Uninstall();
        testCert2.Uninstall();
        testCert3.Uninstall();
    }

    BOOST_AUTO_TEST_SUITE_END()
} 


void AccountHelperTest::TraceAccountNames(
    map<wstring, wstring> const & accountNames)
{
    wstring traceInfo;
    StringWriter writer(traceInfo);
    for (auto const & entry : accountNames)
    {
        writer.Write("{0} - {1} ", entry.first, entry.second);
    }

    Trace.WriteInfo(TraceType, "GetAccountNamesFromX509CommonName: Found {0} account names: {1}", accountNames.size(), traceInfo);
}
