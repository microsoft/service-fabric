// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TestCommon.h"
#include "Common/CryptoTest.h"

using namespace Common;
using namespace std;

namespace Transport
{
    class SecuritySettingsTest
    {
        // Round trip test with three things that affect extension allocation in public types
    protected:
        void RoundTripTest(
            wstring const & certIssuerThumbprints,
            wstring const & remoteCertThumbprnits,
            wstring const & findValueSecondary,
            X509FindType::Enum findType = X509FindType::FindByThumbprint);

        SecurityTestSetup securityTestSetup_;
    };

    BOOST_FIXTURE_TEST_SUITE2(SecuritySettingsSuite,SecuritySettingsTest)

    BOOST_AUTO_TEST_CASE(FindByExtensionTest)
    {
        ENTER;

        X509FindValue::SPtr findValue;
        wstring nameValue = L"TestDnsName.microsoft.com";
        wstring findValueString = L"2.5.29.17:3=" + nameValue;
        auto error = X509FindValue::Create(X509FindType::FindByExtension, findValueString, findValue);
        VERIFY_IS_TRUE(error.IsSuccess());

        CERT_ALT_NAME_ENTRY const & certAltName = *((CERT_ALT_NAME_ENTRY const *)findValue->Value());
        VERIFY_IS_TRUE(certAltName.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(certAltName.pwszDNSName, nameValue.c_str()));

        wstring invalidFindValue = L"2.5.29.17:2=" + nameValue;
        error = X509FindValue::Create(X509FindType::FindByExtension, invalidFindValue, findValue);
        VERIFY_IS_TRUE(error.IsError(ErrorCode::FromNtStatus(STATUS_NOT_SUPPORTED).ReadValue()));

        wstring invalidFindValue2 = L"2.5.29.17=xyz" + nameValue;
        error = X509FindValue::Create(X509FindType::FindByExtension, invalidFindValue2, findValue);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

        wstring invalidFindValue3 = L"2.5.29.17:3:xyz" + nameValue;
        error = X509FindValue::Create(X509FindType::FindByExtension, invalidFindValue3, findValue);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

        wstring invalidFindValue4 = L"2.5.29.8:3=" + nameValue;
        error = X509FindValue::Create(X509FindType::FindByExtension, invalidFindValue4, findValue);
        VERIFY_IS_TRUE(error.IsError(ErrorCode::FromNtStatus(STATUS_NOT_SUPPORTED).ReadValue()));

        SecuritySettings secSettings = TTestUtil::CreateX509Settings3(
            findValueString,
            nameValue,
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1");

        VERIFY_IS_TRUE(secSettings.X509FindValue()->Type() == X509FindType::FindByExtension);
        CERT_ALT_NAME_ENTRY const & certAltName2 = *((CERT_ALT_NAME_ENTRY const *)secSettings.X509FindValue()->Value());
        VERIFY_IS_TRUE(certAltName2.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(certAltName2.pwszDNSName, nameValue.c_str()));

        ScopedHeap heap;
        FABRIC_SECURITY_CREDENTIALS credentails;
        secSettings.ToPublicApi(heap, credentails);
        VERIFY_IS_TRUE(credentails.Kind == FABRIC_SECURITY_CREDENTIAL_KIND_X509);
        FABRIC_X509_CREDENTIALS const & x509Credentials = *((FABRIC_X509_CREDENTIALS const *)credentails.Value);
        VERIFY_IS_TRUE(x509Credentials.FindType == FABRIC_X509_FIND_TYPE_FINDBYEXTENSION);
        CERT_ALT_NAME_ENTRY const & certAltName3 = *((CERT_ALT_NAME_ENTRY const *)x509Credentials.FindValue);
        VERIFY_IS_TRUE(certAltName3.dwAltNameChoice == CERT_ALT_NAME_DNS_NAME);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(certAltName3.pwszDNSName, nameValue.c_str()));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest0)
    {
        ENTER;
        RoundTripTest(
            L"",
            L"",
            L"");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest1)
    {
        ENTER;
        RoundTripTest(
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1",
            L"",
            L"");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest2)
    {
        ENTER;
        RoundTripTest(
            L"",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f2",
            L"");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest3)
    {
        ENTER;
        RoundTripTest(
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f3",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f2",
            L"");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest4)
    {
        ENTER;
        RoundTripTest(
            L"",
            L"",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f4");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest5)
    {
        ENTER;
        RoundTripTest(
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1",
            L"",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f4");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest6)
    {
        ENTER;
        RoundTripTest(
            L"",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f2",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f4");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest7)
    {
        ENTER;
        RoundTripTest(
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f2",
            L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f4");
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest8)
    {
        ENTER;
        RoundTripTest(L"", L"", L"", X509FindType::FindBySubjectName);
        RoundTripTest(L"", L"", L"CN=NameSecondary", X509FindType::FindBySubjectName);
        RoundTripTest(L"", L"", L"NameSecondary", X509FindType::FindBySubjectName);
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest9)
    {
        ENTER;

        SecuritySettings fromConfig = TTestUtil::CreateX509Settings(
            L"CN=SomeName",
            L"rn1,RN2,rN3,Rn4,rn5",
            L"a3449b018d0f6839a2c5d62b5b6c6ac822b6f662,b3449b018d0f6839a2c5d62b5b6c6ac822b6f662,b3449b018d0f6839a2c5d62b5b6c6ac822b6f663");

        Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        fromConfig.ToPublicApi(heap, publicType);

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);

        VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ThumprintNullTest)
    {
        ENTER;

        ThumbprintSet thumbprintSet;
        auto error = thumbprintSet.Add(nullptr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ArgumentNull));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest9_2)
    {
        ENTER;

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        SecuritySettings fromConfig2;
        {
            // create fromConfig in inner scope to detect missing memory allocation in ToPublicApi
            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(
                L"CN=SomeName",
                L"rn1,RN2,rN3,Rn4,rn5",
                L"a3449b018d0f6839a2c5d62b5b6c6ac822b6f662,b3449b018d0f6839a2c5d62b5b6c6ac822b6f662,b3449b018d0f6839a2c5d62b5b6c6ac822b6f663");

            fromConfig2 = fromConfig;

            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);
            fromConfig.ToPublicApi(heap, publicType);
        }

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
        VERIFY_ARE_EQUAL2(fromConfig2, fromPublicType);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest10)
    {
        ENTER;

        vector<wstring> subjectAltNames;
        subjectAltNames.push_back(L"TestDns.TestSubDomain.Microsoft.Com");

        SecuritySettings fromConfig = TTestUtil::CreateX509SettingsBySan(
            X509Default::StoreName(),
            subjectAltNames.front(),
            subjectAltNames.front());

        Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        fromConfig.ToPublicApi(heap, publicType);

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        Trace.WriteInfo(TraceType, "SecuritySettings::FromPublicApi={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);

        VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest10_2)
    {
        ENTER;

        vector<wstring> subjectAltNames;
        subjectAltNames.push_back(L"TestDns.TestSubDomain.Microsoft.Com");

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        {
            // create fromConfig in inner scope to detect missing memory allocation in ToPublicApi
            SecuritySettings fromConfig = TTestUtil::CreateX509SettingsBySan(
                X509Default::StoreName(),
                subjectAltNames.front(),
                subjectAltNames.front());

            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);
            fromConfig.ToPublicApi(heap, publicType);
        }

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        Trace.WriteInfo(TraceType, "SecuritySettings::FromPublicApi={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);

        SecuritySettings fromConfig2 = TTestUtil::CreateX509SettingsBySan(
            X509Default::StoreName(),
            subjectAltNames.front(),
            subjectAltNames.front());

        VERIFY_ARE_EQUAL2(fromConfig2, fromPublicType);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoundTripTest10_3)
    {
        ENTER;

        vector<wstring> subjectAltNames;
        subjectAltNames.push_back(L"TestDns.TestSubDomain.Microsoft.Com");

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        SecuritySettings fromConfig2;
        {
            // create fromConfig in inner scope to detect missing memory allocation in ToPublicApi
            SecuritySettings fromConfig = TTestUtil::CreateX509SettingsBySan(
                X509Default::StoreName(),
                subjectAltNames.front(),
                subjectAltNames.front());

            fromConfig2 = fromConfig;

            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);
            fromConfig.ToPublicApi(heap, publicType);
        }

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        Trace.WriteInfo(TraceType, "SecuritySettings::FromPublicApi={0}", error);
        VERIFY_IS_TRUE(error.IsSuccess());
        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);

        VERIFY_ARE_EQUAL2(fromConfig2, fromPublicType);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameMapTest)
    {
        ENTER;

        Thumbprint::SPtr certIssuerThumbprint1;
        auto err = Thumbprint::Create(L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1", certIssuerThumbprint1);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);
        Thumbprint::SPtr certIssuerThumbprint2;
        err = Thumbprint::Create(L"b3449b018d0f6839a2c5d62b5b6c6ac822b6f662", certIssuerThumbprint2);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);
        Thumbprint::SPtr certIssuerThumbprint3;
        err = Thumbprint::Create(L"a3449b018d0f6839a2c5d62b5b6c6ac822b6f662", certIssuerThumbprint3);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);

        SecurityConfig::X509NameMap remoteNames;
        remoteNames.Add(L"RemoteNameWithNoIssuerPinning");
        remoteNames.Add(L"RemoteNameWithOneIssuer", wformatString(certIssuerThumbprint2));
        remoteNames.Add(L"RemoteNameWithTwoIssuers", wformatString(certIssuerThumbprint3) + L"," + wformatString(certIssuerThumbprint2));

        SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames);
        Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

        Trace.WriteInfo(TraceType, "test name and issuer matching");
        SecurityConfig::X509NameMapBase::const_iterator match;
        bool matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithNoIssuerPinning",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.IsEmpty());

        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_FALSE(matched);


        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint2,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.Size() == 1);
        VERIFY_IS_TRUE(match->second.Contains(certIssuerThumbprint2));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint1));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint3));

        match = fromConfig.RemoteX509Names().Find(L"RemoteNameWithTwoIssuers");
        VERIFY_IS_TRUE(match != fromConfig.RemoteX509Names().CEnd());
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint2, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint3, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint1, match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint1,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint2,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint3,
            match);
        VERIFY_IS_FALSE(matched);

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        fromConfig.ToPublicApi(heap, publicType);

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
        VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
        VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 3);

        Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithNoIssuerPinning",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.IsEmpty());

        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_FALSE(matched);


        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint2,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.Size() == 1);
        VERIFY_IS_TRUE(match->second.Contains(certIssuerThumbprint2));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint1));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint3));

        match = fromConfig.RemoteX509Names().Find(L"RemoteNameWithTwoIssuers");
        VERIFY_IS_TRUE(match != fromConfig.RemoteX509Names().CEnd());
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint2, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint3, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint1, match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint1,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint2,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint3,
            match);
        VERIFY_IS_FALSE(matched);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameMapTest2)
    {
        ENTER;

        Thumbprint::SPtr certIssuerThumbprint1;
        auto err = Thumbprint::Create(L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1", certIssuerThumbprint1);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);
        Thumbprint::SPtr certIssuerThumbprint2;
        err = Thumbprint::Create(L"b3449b018d0f6839a2c5d62b5b6c6ac822b6f662", certIssuerThumbprint2);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);
        Thumbprint::SPtr certIssuerThumbprint3;
        err = Thumbprint::Create(L"a3449b018d0f6839a2c5d62b5b6c6ac822b6f662", certIssuerThumbprint3);
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);

        SecurityConfig::X509NameMap remoteNames;
        remoteNames.Add(L"RemoteNameWithNoIssuerPinning");
        remoteNames.Add(L"RemoteNameWithOneIssuer", wformatString(certIssuerThumbprint2));
        remoteNames.Add(L"RemoteNameWithTwoIssuers", wformatString(certIssuerThumbprint3) + L"," + wformatString(certIssuerThumbprint2));

        SecuritySettings fromConfig = TTestUtil::CreateX509Settings2(L"c3449b018d0f6839a2c5d62b5b6c6ac822b6f662", remoteNames);
        Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

        Trace.WriteInfo(TraceType, "test name and issuer matching");
        SecurityConfig::X509NameMapBase::const_iterator match;
        bool matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithNoIssuerPinning",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.IsEmpty());

        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_FALSE(matched);


        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint2,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.Size() == 1);
        VERIFY_IS_TRUE(match->second.Contains(certIssuerThumbprint2));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint1));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint3));

        match = fromConfig.RemoteX509Names().Find(L"RemoteNameWithTwoIssuers");
        VERIFY_IS_TRUE(match != fromConfig.RemoteX509Names().CEnd());
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint2, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint3, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint1, match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint1,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint2,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint3,
            match);
        VERIFY_IS_FALSE(matched);

        FABRIC_SECURITY_CREDENTIALS publicType = {};
        ScopedHeap heap;
        fromConfig.ToPublicApi(heap, publicType);

        SecuritySettings fromPublicType;
        auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
        VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
        VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 3);

        Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithNoIssuerPinning",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.IsEmpty());

        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint1,
            match);

        VERIFY_IS_FALSE(matched);


        matched = fromConfig.RemoteX509Names().Match(
            L"RemoteNameWithOneIssuer",
            certIssuerThumbprint2,
            match);

        VERIFY_IS_TRUE(matched);
        VERIFY_IS_TRUE(match->second.Size() == 1);
        VERIFY_IS_TRUE(match->second.Contains(certIssuerThumbprint2));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint1));
        VERIFY_IS_FALSE(match->second.Contains(certIssuerThumbprint3));

        match = fromConfig.RemoteX509Names().Find(L"RemoteNameWithTwoIssuers");
        VERIFY_IS_TRUE(match != fromConfig.RemoteX509Names().CEnd());
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint2, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint3, match);
        VERIFY_IS_TRUE(matched);
        matched = fromConfig.RemoteX509Names().MatchIssuer(certIssuerThumbprint1, match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint1,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint2,
            match);
        VERIFY_IS_FALSE(matched);

        matched = fromConfig.RemoteX509Names().Match(
            L"NoSuchNameInTheMap",
            certIssuerThumbprint3,
            match);
        VERIFY_IS_FALSE(matched);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FromPublicApiTest)
    {
        ENTER;

        wstring const remoteName0 = L"remoteName0";
        wstring const remoteName1 = L"remoteName1";
        wchar_t* findValue = L"CN=LocalSubjectName";

        FABRIC_X509_CREDENTIALS x509Credentials = {};
        x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
        x509Credentials.FindValue = findValue;
        x509Credentials.StoreName = X509Default::StoreName().c_str();
        x509Credentials.StoreLocation = FABRIC_X509_STORE_LOCATION_LOCALMACHINE;
        x509Credentials.AllowedCommonNameCount = 2;
        LPCWSTR remoteNames[] = { remoteName0.c_str(), remoteName1.c_str() };
        x509Credentials.AllowedCommonNames = remoteNames;

        FABRIC_SECURITY_CREDENTIALS credentials = { FABRIC_SECURITY_CREDENTIAL_KIND_X509, &x509Credentials };

        SecuritySettings settings;
        auto error = SecuritySettings::FromPublicApi(credentials, settings);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(settings.RemoteX509Names().Size() == 2);
        SecurityConfig::X509NameMapBase::const_iterator matched;
        auto thumbprint = make_shared<Thumbprint>();
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0, thumbprint, matched));
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName1, thumbprint, matched));

        VERIFY_IS_TRUE(thumbprint->Initialize(L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1").IsSuccess());
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0, thumbprint, matched));
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName1, thumbprint, matched));

        VERIFY_IS_FALSE(settings.RemoteX509Names().Match(L"NotExists", thumbprint, matched));

        ScopedHeap heap;
        FABRIC_SECURITY_CREDENTIALS publicApiType = {};
        settings.ToPublicApi(heap, publicApiType);

        VERIFY_IS_TRUE(publicApiType.Kind == credentials.Kind);

        FABRIC_X509_CREDENTIALS const & x509CredentialsRef = *((FABRIC_X509_CREDENTIALS const *)(publicApiType.Value));
        VERIFY_IS_TRUE(x509CredentialsRef.FindType == x509Credentials.FindType);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive((LPCWSTR)(x509CredentialsRef.FindValue), (LPCWSTR)(x509Credentials.FindValue)));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.StoreName, x509Credentials.StoreName));
        VERIFY_IS_TRUE(x509CredentialsRef.StoreLocation == x509Credentials.StoreLocation);
        VERIFY_IS_TRUE(x509CredentialsRef.AllowedCommonNameCount == 0);
        VERIFY_IS_TRUE(x509CredentialsRef.Reserved != nullptr);

        FABRIC_X509_CREDENTIALS_EX1 const & x509Ex1 = *((FABRIC_X509_CREDENTIALS_EX1 const*)(x509CredentialsRef.Reserved));
        VERIFY_IS_TRUE(x509Ex1.IssuerThumbprintCount == 0);
        VERIFY_IS_TRUE(x509Ex1.Reserved != nullptr);

        FABRIC_X509_CREDENTIALS_EX2 const & x509Ex2 = *((FABRIC_X509_CREDENTIALS_EX2 const*)(x509Ex1.Reserved));
        VERIFY_IS_TRUE(x509Ex2.RemoteCertThumbprintCount == 0);
        VERIFY_IS_TRUE(x509Ex2.FindValueSecondary == nullptr);
        VERIFY_IS_TRUE(x509Ex2.RemoteX509NameCount == 2);

        bool b0 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[0].Name, x509Credentials.AllowedCommonNames[0]);
        bool b1 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[1].Name, x509Credentials.AllowedCommonNames[1]);
        bool b2 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[0].Name, x509Credentials.AllowedCommonNames[1]);
        bool b3 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[1].Name, x509Credentials.AllowedCommonNames[0]);
        Trace.WriteInfo(TraceType, "b: {0}, {1}, {2}, {3}", b0, b1, b2, b3);
        VERIFY_IS_TRUE((b0 && b1) || (b2 && b3));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(FromPublicApiTestWithIssuers)
    {
        ENTER;

        wstring const remoteName0 = L"remoteName0";
        wstring const remoteName1 = L"remoteName1";
        wchar_t* findValue = L"CN=LocalSubjectName";

        FABRIC_X509_CREDENTIALS x509Credentials = {};
        x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
        x509Credentials.FindValue = findValue; 
        x509Credentials.StoreName = X509Default::StoreName().c_str();
        x509Credentials.StoreLocation = FABRIC_X509_STORE_LOCATION_LOCALMACHINE;
        x509Credentials.AllowedCommonNameCount = 2;
        LPCWSTR remoteNames[] = { remoteName0.c_str(), remoteName1.c_str() };
        x509Credentials.AllowedCommonNames = remoteNames;

        ScopedHeap h;
        ReferencePointer<FABRIC_X509_CREDENTIALS_EX1> x509Ex1RPtr = h.AddItem<FABRIC_X509_CREDENTIALS_EX1>();
        ReferencePointer<FABRIC_X509_CREDENTIALS_EX2> x509Ex2RPtr = h.AddItem<FABRIC_X509_CREDENTIALS_EX2>();
        ReferencePointer<FABRIC_X509_CREDENTIALS_EX3> x509Ex3RPtr = h.AddItem<FABRIC_X509_CREDENTIALS_EX3>();

        x509Credentials.Reserved = x509Ex1RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX1 & ex1 = *x509Ex1RPtr;

        ex1.Reserved = x509Ex2RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX2 & ex2 = *((FABRIC_X509_CREDENTIALS_EX2*)(ex1.Reserved));

        ex2.Reserved = x509Ex3RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX3 & ex3 = *((FABRIC_X509_CREDENTIALS_EX3*)(ex2.Reserved));
        
        FABRIC_X509_ISSUER_NAME issuer1 = {};
        issuer1.Name = L"issuer1";
        LPCWSTR issuerStores1[] = { L"Root", L"My" };
        issuer1.IssuerStores = issuerStores1;
        issuer1.IssuerStoreCount = 2;

        FABRIC_X509_ISSUER_NAME issuer2 = {};
        issuer2.Name = L"issuer2";
        LPCWSTR issuerStores2[] = { L"Root" };
        issuer2.IssuerStores = issuerStores2;
        issuer2.IssuerStoreCount = 1;

        FABRIC_X509_ISSUER_NAME issuers[] = { issuer1, issuer2 };
        ex3.RemoteCertIssuers = issuers;
        ex3.RemoteCertIssuerCount = 2;

        FABRIC_SECURITY_CREDENTIALS credentials = { FABRIC_SECURITY_CREDENTIAL_KIND_X509, &x509Credentials };

        SecuritySettings settings;
        auto error = SecuritySettings::FromPublicApi(credentials, settings);
        VERIFY_IS_TRUE(error.IsSuccess());

        VERIFY_IS_TRUE(settings.RemoteX509Names().Size() == 2);
        SecurityConfig::X509NameMapBase::const_iterator matched;
        auto thumbprint = make_shared<Thumbprint>();
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0, thumbprint, matched));
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName1, thumbprint, matched));

        VERIFY_IS_TRUE(thumbprint->Initialize(L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1").IsSuccess());
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0, thumbprint, matched));
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName1, thumbprint, matched));

        VERIFY_IS_FALSE(settings.RemoteX509Names().Match(L"NotExists", thumbprint, matched));

        VERIFY_IS_TRUE(settings.RemoteCertIssuers().Size() == 2);
        VERIFY_IS_TRUE(settings.RemoteCertIssuers().Find(L"issuer1") != settings.RemoteCertIssuers().CEnd());
        VERIFY_IS_TRUE(settings.RemoteCertIssuers().Find(L"issuer2") != settings.RemoteCertIssuers().CEnd());
        VERIFY_IS_FALSE(settings.RemoteCertIssuers().Find(L"not exists") != settings.RemoteCertIssuers().CEnd());

        ScopedHeap heap;
        FABRIC_SECURITY_CREDENTIALS publicApiType = {};
        settings.ToPublicApi(heap, publicApiType);

        VERIFY_IS_TRUE(publicApiType.Kind == credentials.Kind);

        FABRIC_X509_CREDENTIALS const & x509CredentialsRef = *((FABRIC_X509_CREDENTIALS const *)(publicApiType.Value));
        VERIFY_IS_TRUE(x509CredentialsRef.FindType == x509Credentials.FindType);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive((LPCWSTR)(x509CredentialsRef.FindValue), (LPCWSTR)(x509Credentials.FindValue)));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.StoreName, x509Credentials.StoreName));
        VERIFY_IS_TRUE(x509CredentialsRef.StoreLocation == x509Credentials.StoreLocation);
        VERIFY_IS_TRUE(x509CredentialsRef.AllowedCommonNameCount == 0);
        VERIFY_IS_TRUE(x509CredentialsRef.Reserved != nullptr);

        FABRIC_X509_CREDENTIALS_EX1 const & x509Ex1 = *((FABRIC_X509_CREDENTIALS_EX1 const*)(x509CredentialsRef.Reserved));
        VERIFY_IS_TRUE(x509Ex1.IssuerThumbprintCount == 0);
        VERIFY_IS_TRUE(x509Ex1.Reserved != nullptr);

        FABRIC_X509_CREDENTIALS_EX2 const & x509Ex2 = *((FABRIC_X509_CREDENTIALS_EX2 const*)(x509Ex1.Reserved));
        VERIFY_IS_TRUE(x509Ex2.RemoteCertThumbprintCount == 0);
        VERIFY_IS_TRUE(x509Ex2.FindValueSecondary == nullptr);
        VERIFY_IS_TRUE(x509Ex2.RemoteX509NameCount == 2);
        VERIFY_IS_TRUE(x509Ex2.Reserved != nullptr);

        bool b0 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[0].Name, x509Credentials.AllowedCommonNames[0]);
        bool b1 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[1].Name, x509Credentials.AllowedCommonNames[1]);
        bool b2 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[0].Name, x509Credentials.AllowedCommonNames[1]);
        bool b3 = StringUtility::AreEqualCaseInsensitive(x509Ex2.RemoteX509Names[1].Name, x509Credentials.AllowedCommonNames[0]);
        Trace.WriteInfo(TraceType, "b: {0}, {1}, {2}, {3}", b0, b1, b2, b3);
        VERIFY_IS_TRUE((b0 && b1) || (b2 && b3));

        FABRIC_X509_CREDENTIALS_EX3 const & x509Ex3 = *((FABRIC_X509_CREDENTIALS_EX3 const*)(x509Ex2.Reserved));
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuerCount == 2);
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuers[0].IssuerStoreCount == 2);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[0].Name, L"issuer1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[0].IssuerStores[0], L"My"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[0].IssuerStores[1], L"Root"));        
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuers[1].IssuerStoreCount == 1);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[1].Name, L"issuer2"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[1].IssuerStores[0], L"Root"));
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(PublicApiTestWithNullThumbprint)
    {
        ENTER;
        
        wstring const remoteName0 = L"remoteName0";
        wstring const remoteName1 = L"remoteName1";
        wchar_t* findValue = L"CN=LocalSubjectName";
        FABRIC_X509_CREDENTIALS x509Credentials = {};
        x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
        x509Credentials.FindValue = findValue;
        x509Credentials.StoreName = X509Default::StoreName().c_str();
        x509Credentials.StoreLocation = FABRIC_X509_STORE_LOCATION_LOCALMACHINE;
        x509Credentials.AllowedCommonNameCount = 2;
        LPCWSTR remoteNames[] = { remoteName0.c_str(), remoteName1.c_str() };
        x509Credentials.AllowedCommonNames = remoteNames;
        ScopedHeap h;
        ReferencePointer<FABRIC_X509_CREDENTIALS_EX2> x509Ex2RPtr = h.AddItem<FABRIC_X509_CREDENTIALS_EX2>();
        ReferencePointer<FABRIC_X509_CREDENTIALS_EX1> x509Ex1RPtr = h.AddItem<FABRIC_X509_CREDENTIALS_EX1>();
        x509Ex1RPtr->Reserved = x509Ex2RPtr.GetRawPointer();
        x509Credentials.Reserved = x509Ex1RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX1 & ex1 = *x509Ex1RPtr;
        FABRIC_X509_CREDENTIALS_EX2 & ex2 = *((FABRIC_X509_CREDENTIALS_EX2*)(ex1.Reserved));
        ex2.RemoteCertThumbprintCount = 1;
        //Null Thumbprint
        LPCWSTR remoteCertThumbprints[] = { nullptr };
        ex2.RemoteCertThumbprints = remoteCertThumbprints;
        FABRIC_SECURITY_CREDENTIALS credentials = { FABRIC_SECURITY_CREDENTIAL_KIND_X509, &x509Credentials };
        SecuritySettings settings;
        auto error = SecuritySettings::FromPublicApi(credentials, settings);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ArgumentNull));

        LEAVE;
    }

#ifdef PLATFORM_UNIX

    BOOST_AUTO_TEST_CASE(FromPublicApiTest2)
    {
        ENTER;

        InstallTestCertInScope localCert;

        FABRIC_X509_NAME remoteName0 = { L"remoteName0", nullptr };
        FABRIC_X509_NAME remoteName1 = { L"remoteName1", L"fffffffffffffffffffffffffffffffffffffff1" };
        FABRIC_X509_NAME remoteNames[] = {remoteName0, remoteName1 };

        FABRIC_X509_CREDENTIALS2 x509Credentials = {};

        auto certLoadPath = StringUtility::Utf8ToUtf16(localCert.X509ContextRef().FilePath());
        x509Credentials.CertLoadPath = certLoadPath.c_str();

        x509Credentials.RemoteCertThumbprintCount = 1;
        LPCWSTR remoteCertThumbprints[] = { L"fffffffffffffffffffffffffffffffffffffff2" };
        x509Credentials.RemoteCertThumbprints = remoteCertThumbprints;

        x509Credentials.RemoteX509NameCount = ARRAYSIZE(remoteNames);
        x509Credentials.RemoteX509Names = remoteNames;

        x509Credentials.ProtectionLevel = FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN;

        FABRIC_SECURITY_CREDENTIALS credentials = { FABRIC_SECURITY_CREDENTIAL_KIND_X509_2, &x509Credentials };

        SecuritySettings settings;
        auto error = SecuritySettings::FromPublicApi(credentials, settings);
        VERIFY_IS_TRUE(error.IsSuccess());

        CertContextUPtr certLoaded;
        error = CryptoUtility::GetCertificate(
            settings.X509StoreLocation(),
            settings.X509StoreName(),
            nullptr,
            certLoaded);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certLoaded);

        VERIFY_IS_TRUE(settings.RemoteX509Names().Size() == 2);
        SecurityConfig::X509NameMapBase::const_iterator matched;
        auto thumbprint = make_shared<Thumbprint>();
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0.Name, thumbprint, matched));
        VERIFY_IS_FALSE(settings.RemoteX509Names().Match(remoteName1.Name, thumbprint, matched));

        VERIFY_IS_TRUE(thumbprint->Initialize(remoteName1.IssuerCertThumbprint).IsSuccess());
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0.Name, thumbprint, matched));
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName1.Name, thumbprint, matched));

        VERIFY_IS_FALSE(settings.RemoteX509Names().Match(L"NotExists", thumbprint, matched));

        ScopedHeap heap;
        FABRIC_SECURITY_CREDENTIALS publicApiType = {};
        settings.ToPublicApi(heap, publicApiType);

        VERIFY_IS_TRUE(publicApiType.Kind == credentials.Kind);

        auto const & x509CredentialsRef = *((FABRIC_X509_CREDENTIALS2 const *)(publicApiType.Value));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.CertLoadPath, x509Credentials.CertLoadPath));
        VERIFY_IS_TRUE(x509CredentialsRef.RemoteCertThumbprintCount == x509Credentials.RemoteCertThumbprintCount);

        Trace.WriteInfo(TraceType, "x509CredentialsRef.RemoteCertThumbprints[0] = {0}", x509CredentialsRef.RemoteCertThumbprints[0]);
        Trace.WriteInfo(TraceType, "x509Credentials.RemoteCertThumbprints[0] = {0}", x509Credentials.RemoteCertThumbprints[0]);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.RemoteCertThumbprints[0], x509Credentials.RemoteCertThumbprints[0]));
        VERIFY_IS_TRUE(x509CredentialsRef.RemoteX509NameCount == x509Credentials.RemoteX509NameCount);
        for (int i = 0; i < x509CredentialsRef.RemoteX509NameCount; ++i)
        {
            VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.RemoteX509Names[i].Name, x509Credentials.RemoteX509Names[i].Name));
            if (x509CredentialsRef.RemoteX509Names[i].IssuerCertThumbprint)
            {
                VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.RemoteX509Names[i].IssuerCertThumbprint, x509Credentials.RemoteX509Names[i].IssuerCertThumbprint));
            }
            else
            {
                VERIFY_IS_TRUE(x509Credentials.RemoteX509Names[i].IssuerCertThumbprint == nullptr);
            }
        }

        VERIFY_IS_TRUE(x509CredentialsRef.ProtectionLevel == x509Credentials.ProtectionLevel);
        VERIFY_IS_TRUE(x509CredentialsRef.Reserved != nullptr);
        FABRIC_X509_CREDENTIALS_EX3 const & x509Ex3 = *((FABRIC_X509_CREDENTIALS_EX3 const*)(x509CredentialsRef.Reserved));
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuerCount == 0);
        
        SecuritySettings settings2;
        error = SecuritySettings::FromPublicApi(publicApiType, settings2);
        VERIFY_ARE_EQUAL2(error.ReadValue(), ErrorCodeValue::Success);
        Trace.WriteInfo(TraceType, "settings2 = {0}", settings2);
        VERIFY_IS_TRUE(settings == settings2);

        LEAVE;
    }

  
    BOOST_AUTO_TEST_CASE(FromPublicApiTest2WithIssuers)
    {
        ENTER;

        InstallTestCertInScope localCert;

        FABRIC_X509_NAME remoteName0 = { L"remoteName0", nullptr };
         FABRIC_X509_NAME remoteNames[] = {remoteName0 };

        FABRIC_X509_CREDENTIALS2 x509Credentials = {};

        auto certLoadPath = StringUtility::Utf8ToUtf16(localCert.X509ContextRef().FilePath());
        x509Credentials.CertLoadPath = certLoadPath.c_str();

        x509Credentials.RemoteCertThumbprintCount = 1;
        LPCWSTR remoteCertThumbprints[] = { L"fffffffffffffffffffffffffffffffffffffff2" };
        x509Credentials.RemoteCertThumbprints = remoteCertThumbprints;

        x509Credentials.RemoteX509NameCount = ARRAYSIZE(remoteNames);
        x509Credentials.RemoteX509Names = remoteNames;

        x509Credentials.ProtectionLevel = FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN;

        ScopedHeap h;
        ReferencePointer<FABRIC_X509_CREDENTIALS_EX3> x509Ex3RPtr = h.AddItem<FABRIC_X509_CREDENTIALS_EX3>();
        x509Credentials.Reserved = x509Ex3RPtr.GetRawPointer();
        FABRIC_X509_CREDENTIALS_EX3 & ex3 = *((FABRIC_X509_CREDENTIALS_EX3*)(x509Credentials.Reserved));
        
        FABRIC_X509_ISSUER_NAME issuer1 = {};
        issuer1.Name = L"issuer1";
        LPCWSTR issuerStores1[] = { L"Root", L"My" };
        issuer1.IssuerStores = issuerStores1;
        issuer1.IssuerStoreCount = 2;

        FABRIC_X509_ISSUER_NAME issuer2 = {};
        issuer2.Name = L"issuer2";
        LPCWSTR issuerStores2[] = { L"Root" };
        issuer2.IssuerStores = issuerStores2;
        issuer2.IssuerStoreCount = 1;

        FABRIC_X509_ISSUER_NAME issuers[] = { issuer1, issuer2 };
        ex3.RemoteCertIssuers = issuers;
        ex3.RemoteCertIssuerCount = 2;
      
        FABRIC_SECURITY_CREDENTIALS credentials = { FABRIC_SECURITY_CREDENTIAL_KIND_X509_2, &x509Credentials };

        SecuritySettings settings;
        auto error = SecuritySettings::FromPublicApi(credentials, settings);
        VERIFY_IS_TRUE(error.IsSuccess());

        CertContextUPtr certLoaded;
        error = CryptoUtility::GetCertificate(
            settings.X509StoreLocation(),
            settings.X509StoreName(),
            nullptr,
            certLoaded);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(certLoaded);

        VERIFY_IS_TRUE(settings.RemoteX509Names().Size() == 1);
        SecurityConfig::X509NameMapBase::const_iterator matched;
        auto thumbprint = make_shared<Thumbprint>();
        Trace.WriteInfo(TraceType, "matching with issuer thumbprint '{0}'", thumbprint);
        VERIFY_IS_TRUE(settings.RemoteX509Names().Match(remoteName0.Name, thumbprint, matched));
       
        VERIFY_IS_FALSE(settings.RemoteX509Names().Match(L"NotExists", thumbprint, matched));

        VERIFY_IS_TRUE(settings.RemoteCertIssuers().Size() == 2);
        VERIFY_IS_TRUE(settings.RemoteCertIssuers().Find(L"issuer1") != settings.RemoteCertIssuers().CEnd());
        VERIFY_IS_TRUE(settings.RemoteCertIssuers().Find(L"issuer2") != settings.RemoteCertIssuers().CEnd());
        VERIFY_IS_FALSE(settings.RemoteCertIssuers().Find(L"not exists") != settings.RemoteCertIssuers().CEnd());

        ScopedHeap heap;
        FABRIC_SECURITY_CREDENTIALS publicApiType = {};
        settings.ToPublicApi(heap, publicApiType);

        VERIFY_IS_TRUE(publicApiType.Kind == credentials.Kind);

        auto const & x509CredentialsRef = *((FABRIC_X509_CREDENTIALS2 const *)(publicApiType.Value));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.CertLoadPath, x509Credentials.CertLoadPath));
        VERIFY_IS_TRUE(x509CredentialsRef.RemoteCertThumbprintCount == x509Credentials.RemoteCertThumbprintCount);

        Trace.WriteInfo(TraceType, "x509CredentialsRef.RemoteCertThumbprints[0] = {0}", x509CredentialsRef.RemoteCertThumbprints[0]);
        Trace.WriteInfo(TraceType, "x509Credentials.RemoteCertThumbprints[0] = {0}", x509Credentials.RemoteCertThumbprints[0]);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.RemoteCertThumbprints[0], x509Credentials.RemoteCertThumbprints[0]));
        VERIFY_IS_TRUE(x509CredentialsRef.RemoteX509NameCount == x509Credentials.RemoteX509NameCount);
        for (int i = 0; i < x509CredentialsRef.RemoteX509NameCount; ++i)
        {
            VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.RemoteX509Names[i].Name, x509Credentials.RemoteX509Names[i].Name));
            if (x509CredentialsRef.RemoteX509Names[i].IssuerCertThumbprint)
            {
                VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509CredentialsRef.RemoteX509Names[i].IssuerCertThumbprint, x509Credentials.RemoteX509Names[i].IssuerCertThumbprint));
            }
            else
            {
                VERIFY_IS_TRUE(x509Credentials.RemoteX509Names[i].IssuerCertThumbprint == nullptr);
            }
        }

        VERIFY_IS_TRUE(x509CredentialsRef.ProtectionLevel == x509Credentials.ProtectionLevel);
        VERIFY_IS_TRUE(x509CredentialsRef.Reserved != nullptr);

        FABRIC_X509_CREDENTIALS_EX3 const & x509Ex3 = *((FABRIC_X509_CREDENTIALS_EX3 const*)(x509CredentialsRef.Reserved));
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuerCount == 2);
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuers[0].IssuerStoreCount == 2);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[0].Name, L"issuer1"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[0].IssuerStores[0], L"My"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[0].IssuerStores[1], L"Root"));        
        VERIFY_IS_TRUE(x509Ex3.RemoteCertIssuers[1].IssuerStoreCount == 1);
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[1].Name, L"issuer2"));
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(x509Ex3.RemoteCertIssuers[1].IssuerStores[0], L"Root"));
        
        SecuritySettings settings2;
        error = SecuritySettings::FromPublicApi(publicApiType, settings2);
        VERIFY_ARE_EQUAL2(error.ReadValue(), ErrorCodeValue::Success);
        Trace.WriteInfo(TraceType, "settings2 = {0}", settings2);
        VERIFY_IS_TRUE(settings == settings2);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IssuerStoreKeyValueMap_MatchTest)
    {
        ENTER;

        wstring certCName = L"cert1";
        InstallTestCertInScope cert1(L"CN=" + certCName);

        X509PubKey::SPtr certIssuerPubKey = make_shared<X509PubKey>(cert1.CertContext());
        Thumbprint::SPtr certIssuerThumbprint;
        auto err = Thumbprint::Create(cert1.CertContext(), certIssuerThumbprint);
        VERIFY_IS_TRUE(err.IsSuccess());

        Trace.WriteInfo(TraceType, "only add issuer stores");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(certCName, L"");

            SecurityConfig::IssuerStoreKeyValueMap issuerMap;
            issuerMap.Add(certCName, L"My");

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames, issuerMap);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            fromConfig.ToPublicApi(heap, publicType);

            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
            VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 1);

            Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);
        }
        LEAVE;
    }

#else

    BOOST_AUTO_TEST_CASE(X509NameMapTest_IssuerPinning_PubKeyOrThumbprint)
    {
        ENTER;

        wstring cert1CName = L"cert1.X509NameMapTest";
        InstallTestCertInScope cert1(L"CN=" + cert1CName);

        X509PubKey::SPtr cert1IssuerPubKey = make_shared<X509PubKey>(cert1.CertContext());
        Thumbprint::SPtr cert1IssuerThumbprint;
        auto err = Thumbprint::Create(cert1.CertContext(), cert1IssuerThumbprint);
        VERIFY_IS_TRUE(err.IsSuccess());

        Trace.WriteInfo(TraceType, "only add issuer pubkey to allow list");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(cert1CName, cert1IssuerPubKey);

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerPubKey,
                cert1IssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerPubKey,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);
        }

        Trace.WriteInfo(TraceType, "only add issuer certificate thumbprint to allow list");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(cert1CName, cert1IssuerThumbprint);

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerPubKey,
                cert1IssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerPubKey,
                match);

            VERIFY_IS_FALSE(matched);
        }

        Trace.WriteInfo(TraceType, "add both issuer pubkey and issuer certificate thumbprint to allow list");
        {
            SecurityConfig::X509NameMap remoteNames;
            X509IdentitySet cert1IssuerIdSet;
            cert1IssuerIdSet.Add(cert1IssuerPubKey);
            cert1IssuerIdSet.Add(cert1IssuerThumbprint);
            remoteNames.Add(cert1CName, cert1IssuerIdSet);

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerPubKey,
                cert1IssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerPubKey,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromConfig.RemoteX509Names().Match(
                cert1CName,
                cert1IssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IssuerStoreKeyValueMap_MatchTest)
    {
        ENTER;

        std::shared_ptr<Common::X509FindValue> findValue;
        X509FindValue::Create(X509FindType::FindByThumbprint, L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62", findValue);
        CertContextUPtr cert;
        auto error = CryptoUtility::GetCertificate(
            X509StoreLocation::LocalMachine,
            L"Root",
            findValue,
            cert);
        VERIFY_IS_TRUE(error.IsSuccess());
        X509PubKey::SPtr certIssuerPubKey = make_shared<X509PubKey>(cert.get());
        Thumbprint::SPtr certIssuerThumbprint; 
        error = Thumbprint::Create(cert.get(), certIssuerThumbprint);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring certCName = L"WinFabric-Test-SAN1-Alice";
        wstring certIssuerName = L"WinFabric-Test-TA-CA";
        Trace.WriteInfo(TraceType, "only add issuer stores");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(certCName, L"");

            SecurityConfig::IssuerStoreKeyValueMap issuerMap;
            issuerMap.Add(certIssuerName, L"Root");

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames, issuerMap);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            fromConfig.ToPublicApi(heap, publicType);

            SecuritySettings fromPublicType;
            error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
            VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 1);

            Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 1);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);
        }
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IssuerStoreKeyValueMap_MatchTest2)
    {
        ENTER;

        std::shared_ptr<Common::X509FindValue> findValue;
        X509FindValue::Create(X509FindType::FindByThumbprint, L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62", findValue);
        CertContextUPtr cert;
        auto error = CryptoUtility::GetCertificate(
            X509StoreLocation::LocalMachine,
            L"Root",
            findValue,
            cert);
        VERIFY_IS_TRUE(error.IsSuccess());
        X509PubKey::SPtr certIssuerPubKey = make_shared<X509PubKey>(cert.get());
        Thumbprint::SPtr certIssuerThumbprint;
        error = Thumbprint::Create(cert.get(), certIssuerThumbprint);
        VERIFY_IS_TRUE(error.IsSuccess());

        std::shared_ptr<Common::X509FindValue> findValue2;
        X509FindValue::Create(X509FindType::FindByThumbprint, L"bc 21 ae 9f 0b 88 cf 6e a9 b4 d6 23 3f 97 2a 60 63 b2 25 a9", findValue2);
        CertContextUPtr cert2;
        error = CryptoUtility::GetCertificate(
            X509StoreLocation::LocalMachine,
            L"Root",
            findValue,
            cert2);
        VERIFY_IS_TRUE(error.IsSuccess());
        X509PubKey::SPtr certIssuerPubKey2 = make_shared<X509PubKey>(cert2.get());
        Thumbprint::SPtr certIssuerThumbprint2;
        error = Thumbprint::Create(cert2.get(), certIssuerThumbprint2);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring certCName = L"WinFabric-Test-SAN1-Alice";

        wstring certIssuerName = L"WinFabric-Test-TA-CA";
        wstring certIssuerName2 = L"WinFabric-Test-Expired";
        Trace.WriteInfo(TraceType, "only add issuer stores");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(certCName, L"");

            SecurityConfig::IssuerStoreKeyValueMap issuerMap;
            issuerMap.Add(certIssuerName, L"Root");
            issuerMap.Add(certIssuerName2, L"Root");

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames, issuerMap);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey2,
                certIssuerThumbprint2,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey2,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint2,
                match);

            VERIFY_IS_FALSE(matched);

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            fromConfig.ToPublicApi(heap, publicType);

            SecuritySettings fromPublicType;
            error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
            VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 1);

            Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey2,
                certIssuerThumbprint2,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey2,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() == 2);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint2,
                match);

            VERIFY_IS_FALSE(matched);
        }
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IssuerStoreKeyValueMap_MatchTest3)
    {
        ENTER;

        std::shared_ptr<Common::X509FindValue> findValue;
        X509FindValue::Create(X509FindType::FindByThumbprint, L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62", findValue);
        CertContextUPtr cert;
        auto error = CryptoUtility::GetCertificate(
            X509StoreLocation::LocalMachine,
            L"Root",
            findValue,
            cert);
        VERIFY_IS_TRUE(error.IsSuccess());
        X509PubKey::SPtr certIssuerPubKey = make_shared<X509PubKey>(cert.get());
        Thumbprint::SPtr certIssuerThumbprint;
        error = Thumbprint::Create(cert.get(), certIssuerThumbprint);
        VERIFY_IS_TRUE(error.IsSuccess());

        wstring certCName = L"WinFabric-Test-SAN1-Alice";
        Trace.WriteInfo(TraceType, "only add issuer stores");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(certCName, L"");

            // verify for empty issuer store name. In this case all certs in ROOT should be white-listed
            SecurityConfig::IssuerStoreKeyValueMap issuerMap;
            issuerMap.Add(L"", L"Root");

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames, issuerMap);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() > 1);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() > 1);

            matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            fromConfig.ToPublicApi(heap, publicType);

            SecuritySettings fromPublicType;
            error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
            VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 1);

            Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() > 1);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerPubKey,
                match);
            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.Size() > 1);

            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_FALSE(matched);
        }
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(IssuerStoreKeyValueMap_MatchTest4)
    {
        ENTER;

        wstring certCName = L"WinFabric-Test-SAN1-Alice";
        wstring certCNIssuerName = L"Dummy-Issuer-Not-PresentInStore";
        Thumbprint::SPtr certIssuerThumbprint;
        auto err = Thumbprint::Create(L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1", certIssuerThumbprint);
        Trace.WriteInfo(TraceType, "only add issuer stores");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(certCName, L"");

            // Verify if issuer is not present in store. By design we will whitelist everything.
            SecurityConfig::IssuerStoreKeyValueMap issuerMap;
            issuerMap.Add(certCNIssuerName, L"Root");

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames, issuerMap);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            Trace.WriteInfo(TraceType, "test name and issuer matching");
            SecurityConfig::X509NameMapBase::const_iterator match;
            bool matched = fromConfig.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.IsEmpty());

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            fromConfig.ToPublicApi(heap, publicType);

            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
            VERIFY_IS_TRUE(fromPublicType.RemoteX509Names().Size() == 1);

            Trace.WriteInfo(TraceType, "repeat the same match tests after round trip conversion");
            matched = fromPublicType.RemoteX509Names().Match(
                certCName,
                certIssuerThumbprint,
                match);

            VERIFY_IS_TRUE(matched);
            VERIFY_IS_TRUE(match->second.IsEmpty());
        }
        LEAVE;
    }

#endif

    BOOST_AUTO_TEST_CASE(X509FindValueSecondaryTests)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "testing conversion");
        {
            X509FindValue::SPtr findValue;
            wstring const primaryValue = L"primaryValue";
            auto error = X509FindValue::Create(X509FindType::FindBySubjectName, primaryValue, findValue);
            VERIFY_IS_TRUE(error.IsSuccess());

            pair<wstring, wstring> values = findValue->ToStrings();
            VERIFY_ARE_EQUAL2(values.first, primaryValue);
            VERIFY_IS_TRUE(values.second.empty());
        }

        Trace.WriteInfo(TraceType, "testing conversion 2");
        {
            X509FindValue::SPtr findValue;
            wstring const primaryValue = L"primaryValue";
            wstring const secondaryValue = L"secondaryValue";
            auto error = X509FindValue::Create(X509FindType::FindBySubjectName, primaryValue, secondaryValue, findValue);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(findValue->Secondary() != nullptr);

            pair<wstring, wstring> values = findValue->ToStrings();
            Trace.WriteInfo(TraceType, "first = {0}, second = {1}", values.first, values.second);
            VERIFY_ARE_EQUAL2(values.first, primaryValue);
            VERIFY_ARE_EQUAL2(values.second, secondaryValue);
        }

        Trace.WriteInfo(TraceType, "testing conversion 3");
        {
            X509FindValue::SPtr findValue;
            wstring const primaryValue = L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f1";
            wstring const secondaryValue = L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff f2";
            auto error = X509FindValue::Create(X509FindType::FindByThumbprint, primaryValue, secondaryValue, findValue);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(findValue->Secondary() != nullptr);

            pair<wstring, wstring> values = findValue->ToStrings();
            Trace.WriteInfo(TraceType, "first = {0}, second = {1}", values.first, values.second);

            Thumbprint t1;
            error = t1.Initialize(primaryValue);
            VERIFY_IS_TRUE(error.IsSuccess());
            Thumbprint t1Converted;
            error = t1Converted.Initialize(values.first);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(t1 == t1Converted);

            Thumbprint t2;
            error = t2.Initialize(secondaryValue);
            VERIFY_IS_TRUE(error.IsSuccess());
            Thumbprint t2Converted;
            error = t2Converted.Initialize(values.second);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(t2 == t2Converted);
        }

        Trace.WriteInfo(TraceType, "testing secondary subject name");
        {
            wstring commonNameExpected = L"WinFabric-Test-SAN1-Alice";
            SecuritySettings sslSettings = TTestUtil::CreateX509Settings(
                L"CN=NoSuchNameExistOrWeFail",
                L"CN=" + commonNameExpected,
                L"AnyNonEmptyStringIsFine",
                L"");

            Trace.WriteInfo(TraceType, "sslSettings={0}", sslSettings);

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            sslSettings.ToPublicApi(heap, publicType);
            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);

            CertContextUPtr certificate;
            error = CryptoUtility::GetCertificate(
                X509Default::StoreLocation(), 
                X509Default::StoreName(),
                fromPublicType.X509FindValue(),
                certificate);

            // Falling back on secondary only happens with credential loading
            VERIFY_IS_FALSE(error.IsSuccess());
        }

        Trace.WriteInfo(TraceType, "testing secondary thumbprint");
        {
            wstring expectedThumbprintString = L"78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf";
            SecuritySettings sslSettings = TTestUtil::CreateX509Settings2(
                L"ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff",
                expectedThumbprintString,
                L"AnyNonEmptyStringIsFine",
                L"");

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            sslSettings.ToPublicApi(heap, publicType);
            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            CertContextUPtr certificate;
            error = CryptoUtility::GetCertificate(
                X509Default::StoreLocation(), 
                X509Default::StoreName(),
                fromPublicType.X509FindValue(),
                certificate);

            // Falling back on secondary only happens with credential loading
            VERIFY_IS_FALSE(error.IsSuccess());
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SslEqualityTest3)
    {
        ENTER;

        wstring certCName = L"WinFabric-Test-SAN1-Alice";
        wstring certIssuerName = L"WinFabric-Test-TA-CA";
        Trace.WriteInfo(TraceType, "only add issuer stores");
        {
            SecurityConfig::X509NameMap remoteNames;
            remoteNames.Add(certCName, L"");

            SecurityConfig::IssuerStoreKeyValueMap issuerMap;
            issuerMap.Add(certIssuerName, L"Root");

            SecuritySettings fromConfig = TTestUtil::CreateX509Settings(L"CN=LocalCommonName", remoteNames, issuerMap);
            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            SecuritySettings sslSettings = fromConfig;
            VERIFY_IS_TRUE(sslSettings == fromConfig);
        }
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SslEqualityTest2)
    {
        wstring thumbprintStr1 = L"b3 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62";
        wstring thumbprintStr11 = L"b3449b018d0f6839a2c5d62b5b6c6ac822b6f662";
        wstring thumbprintStr2 = L"b4 44 9b 01 8d 0f 68 39 a2 c5 d6 2b 5b 6c 6a c8 22 b6 f6 62";
        wstring thumbprintStr3 = L"85 37 1c a6 e5 50 14 3d ce 28 03 47 1b de 3a 09 e8 f8 77 0f";

        Thumbprint thumbprint1;
        VERIFY_IS_TRUE(thumbprint1.Initialize(thumbprintStr1).IsSuccess());
        VERIFY_IS_TRUE(thumbprint1 == thumbprint1);
        {
            Thumbprint thumbprintTmp(thumbprint1);
            VERIFY_IS_TRUE(thumbprintTmp == thumbprint1);

            Thumbprint thumbprintTmp2(move(thumbprintTmp));
            VERIFY_IS_TRUE(thumbprintTmp2 == thumbprint1);
            VERIFY_IS_FALSE(thumbprintTmp2 == thumbprintTmp);

            Thumbprint thumbprintTmp3;
            VERIFY_IS_TRUE(thumbprintTmp3.Initialize(wformatString(thumbprintTmp2)).IsSuccess());
            VERIFY_IS_TRUE(thumbprintTmp3 == thumbprint1);
        }

        Thumbprint thumbprint11;
        {
            Thumbprint thumbprintTmp3;
            VERIFY_IS_TRUE(thumbprintTmp3.Initialize(thumbprintStr11).IsSuccess());
            thumbprint11 = thumbprintTmp3;
            VERIFY_IS_TRUE(thumbprint11 == thumbprintTmp3);
        }
        VERIFY_IS_TRUE(thumbprint1 == thumbprint11);

        Thumbprint thumbprint2;
        {
            Thumbprint thumbprintTmp4;
            VERIFY_IS_TRUE(thumbprintTmp4.Initialize(thumbprintStr2).IsSuccess());
            thumbprint2 = move(thumbprintTmp4);
            VERIFY_IS_FALSE(thumbprint2 == thumbprintTmp4);
        }
        VERIFY_IS_FALSE(thumbprint1 == thumbprint2);

        Thumbprint::SPtr thumbprint3;
        auto error = Thumbprint::Create(thumbprintStr3, thumbprint3);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_FALSE(*thumbprint3 == thumbprint1);
        VERIFY_IS_FALSE(*thumbprint3 == thumbprint2);

        // Test ThumbprintSet
        ThumbprintSet thumbprintSet;
        VERIFY_IS_TRUE(thumbprintSet.Add(thumbprintStr1).IsSuccess());
        VERIFY_IS_TRUE(thumbprintSet.Value().size() == 1);
        VERIFY_IS_TRUE(thumbprintSet.Contains(thumbprint1));
        VERIFY_IS_TRUE(thumbprintSet.Contains(thumbprint11));
        VERIFY_IS_FALSE(thumbprintSet.Contains(thumbprint2));
        VERIFY_IS_FALSE(thumbprintSet.Contains(*thumbprint3));
        VERIFY_IS_TRUE(thumbprintSet.Add(thumbprintStr11).IsSuccess());
        VERIFY_IS_TRUE(thumbprintSet.Add(thumbprintStr2).IsSuccess());
        VERIFY_IS_TRUE(thumbprintSet.Contains(thumbprint2));
        VERIFY_IS_FALSE(thumbprintSet.Contains(*thumbprint3));
        VERIFY_IS_TRUE(thumbprintSet.Add(thumbprintStr3).IsSuccess());
        VERIFY_IS_TRUE(thumbprintSet.Contains(*thumbprint3));

        // Test on SSL settings
        SecuritySettings sslSettings0 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice", L"RemoteName", thumbprintStr1);

        SecuritySettings sslSettings = sslSettings0;
        VERIFY_IS_TRUE(sslSettings == sslSettings0);

        VERIFY_IS_TRUE(sslSettings.RemoteX509Names().Size() == 1);
        auto thumbprint1SPtr = make_shared<Thumbprint>(thumbprint1);
        VERIFY_IS_TRUE(sslSettings.RemoteX509Names().CBegin()->second.Contains(thumbprint1SPtr));
        auto thumbprint2SPtr = make_shared<Thumbprint>(thumbprint2);
        VERIFY_IS_TRUE(!sslSettings.RemoteX509Names().CBegin()->second.Contains(thumbprint2SPtr));

        SecuritySettings sslSettings2 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice", L"RemoteName", thumbprintStr11);
        VERIFY_IS_TRUE(sslSettings2 == sslSettings0);

        SecuritySettings sslSettings3 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice", L"RemoteName", thumbprintStr2);
        VERIFY_IS_TRUE(sslSettings3 != sslSettings0);
        VERIFY_IS_TRUE(sslSettings3 != sslSettings2);

        SecuritySettings sslSettings4 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"RemoteName",
            thumbprintStr11 + L"," + thumbprintStr3);

        VERIFY_IS_TRUE(sslSettings4 != sslSettings0);
        VERIFY_IS_TRUE(sslSettings4 != sslSettings2);
        VERIFY_IS_TRUE(sslSettings4 != sslSettings3);

        SecuritySettings sslSettings5 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"RemoteName",
            thumbprintStr1);

        SecuritySettings sslSettings6 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"RemoteName",
            thumbprintStr2);

        VERIFY_IS_TRUE(sslSettings5 != sslSettings6);

        SecuritySettings sslSettings7 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"rn1,rn2,rn3",
            thumbprintStr1);

        SecuritySettings sslSettings8 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"rn2,rn1,rn3",
            thumbprintStr11);

        VERIFY_IS_TRUE(sslSettings7 == sslSettings8);

        // Test on claims settings
        SecuritySettings claimsSettings;
        VERIFY_IS_TRUE(SecuritySettings::CreateClaimTokenClient(L"t1=v1", thumbprintStr1, L"someName", L"", wformatString(ProtectionLevel::EncryptAndSign), claimsSettings).IsSuccess());

        // Test on Kerberos settings
        SecuritySettings kerbSettings;
        VERIFY_IS_TRUE(SecuritySettings::CreateKerberos(L"someSpn", L"someClient", wformatString(ProtectionLevel::EncryptAndSign), kerbSettings).IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(RoleClaimsTest)
    {
        ENTER;

        SecuritySettings::RoleClaims rc1;
        rc1.AddClaim(L"t1", L"v1");
        rc1.AddClaim(L"t1", L"V2");
        rc1.AddClaim(L"T2", L"v3");
        rc1.AddClaim(L"t1", L"v1");
        Trace.WriteInfo(TraceType, "rc1: {0}", rc1);

        SecuritySettings::RoleClaims rc2;
        rc2.AddClaim(L"t2", L"V3");
        rc2.AddClaim(L"T1", L"v2");
        rc2.AddClaim(L"t1", L"v1");
        Trace.WriteInfo(TraceType, "rc2: {0}", rc2);

        VERIFY_IS_TRUE(rc1 == rc2);

        SecuritySettings::RoleClaims rc3;
        rc3.AddClaim(L"t1", L"V2");
        rc3.AddClaim(L"T2", L"v3");
        Trace.WriteInfo(TraceType, "rc3: {0}", rc3);

        VERIFY_IS_TRUE(rc1 != rc3);
        VERIFY_IS_TRUE(rc2 != rc3);

        SecuritySettings::RoleClaims rc4;
        rc4.AddClaim(L"t2", L"v2");
        Trace.WriteInfo(TraceType, "rc4: {0}", rc4);

        SecuritySettings::RoleClaims rcNull1;
        Trace.WriteInfo(TraceType, "rcNull1: {0}", rcNull1);
        SecuritySettings::RoleClaims rcNull2;
        Trace.WriteInfo(TraceType, "rcNull2: {0}", rcNull2);

        VERIFY_IS_TRUE(rcNull1 == rcNull2);
        VERIFY_IS_TRUE(rcNull1 != rc1);
        VERIFY_IS_TRUE(rcNull1 != rc2);
        VERIFY_IS_TRUE(rcNull1 != rc3);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(RoleClaimsOrListTest)
    {
        ENTER;

        SecuritySettings::RoleClaims rc1;
        rc1.AddClaim(L"t1", L"v1");
        rc1.AddClaim(L"t2", L"v2");

        SecuritySettings::RoleClaimsOrList roleClaimsOrList;
        roleClaimsOrList.AddRoleClaims(rc1);
        Trace.WriteInfo(TraceType, "added rc1({0}), roleClaimsOrList: {1}", rc1, roleClaimsOrList);
        VERIFY_IS_TRUE(roleClaimsOrList.Value().size() == 1);

        roleClaimsOrList.AddRoleClaims(rc1);
        Trace.WriteInfo(TraceType, "added rc1({0}) again, roleClaimsOrList: {0}", rc1, roleClaimsOrList);
        VERIFY_IS_TRUE(roleClaimsOrList.Value().size() == 1);

        SecuritySettings::RoleClaims rc2;
        rc2.AddClaim(L"t2", L"V2");
        roleClaimsOrList.AddRoleClaims(rc2);
        Trace.WriteInfo(TraceType, "added rc2({0}), roleClaimsOrList: {1}", rc2, roleClaimsOrList);
        VERIFY_IS_TRUE(roleClaimsOrList.Value().size() == 1);

        SecuritySettings::RoleClaims rc3;
        rc3.AddClaim(L"T1", L"v1");
        rc3.AddClaim(L"T2", L"V2");
        rc3.AddClaim(L"T3", L"V3");
        roleClaimsOrList.AddRoleClaims(rc3);
        Trace.WriteInfo(TraceType, "added rc3({0}), roleClaimsOrList: {1}", rc3, roleClaimsOrList);
        VERIFY_IS_TRUE(roleClaimsOrList.Value().size() == 1);

        SecuritySettings::RoleClaims rc4;
        rc4.AddClaim(L"t4", L"v4");
        rc4.AddClaim(L"t3", L"v3");
        roleClaimsOrList.AddRoleClaims(rc4);
        Trace.WriteInfo(TraceType, "added rc4({0}), roleClaimsOrList: {1}", rc4, roleClaimsOrList);
        VERIFY_IS_TRUE(roleClaimsOrList.Value().size() == 2);

        SecuritySettings::RoleClaimsOrList roleClaimsOrList2;
        roleClaimsOrList2.AddRoleClaims(rc4);
        roleClaimsOrList2.AddRoleClaims(rc1);
        Trace.WriteInfo(TraceType, "roleClaimsOrList2: {0}", roleClaimsOrList2);
        VERIFY_IS_TRUE(roleClaimsOrList2.Value().size() == 2);
        VERIFY_IS_TRUE(roleClaimsOrList != roleClaimsOrList2);

        roleClaimsOrList2.AddRoleClaims(rc2);
        Trace.WriteInfo(TraceType, "added rc2({0}), roleClaimsOrList2: {1}", rc2, roleClaimsOrList2);
        VERIFY_IS_TRUE(roleClaimsOrList == roleClaimsOrList2);

        SecuritySettings::RoleClaims empty;
        roleClaimsOrList.AddRoleClaims(empty);
        Trace.WriteInfo(TraceType, "added empty({0}), roleClaimsOrList: {1}", empty, roleClaimsOrList);
        VERIFY_IS_TRUE(roleClaimsOrList.Value().size() == 1);
        VERIFY_IS_TRUE(roleClaimsOrList.Contains(empty));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsEqualityTest)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "client settings");
        SecuritySettings clientSettings1;
        auto error = SecuritySettings::CreateClaimTokenClient(
            L"c1=v1",
            L"",
            L"server1,server2",
            L"",
            wformatString(ProtectionLevel::EncryptAndSign),
            clientSettings1);

        VERIFY_IS_TRUE(error.IsSuccess());

        SecuritySettings clientSettings2;
        error = SecuritySettings::CreateClaimTokenClient(
            L"c1=v1",
            L"",
            L"server2,SERVER1",
            L"",
            wformatString(ProtectionLevel::EncryptAndSign),
            clientSettings2);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(clientSettings1 == clientSettings2);

        SecuritySettings clientSettings3;
        error = SecuritySettings::CreateClaimTokenClient(
            L"c1=v1",
            L"",
            L"server2",
            L"",
            wformatString(ProtectionLevel::EncryptAndSign),
            clientSettings3);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(clientSettings1 != clientSettings3);

        SecuritySettings clientSettings4;
        error = SecuritySettings::CreateClaimTokenClient(
            L"c1=v1",
            L"",
            L"server1,server2,server3",
            L"",
            wformatString(ProtectionLevel::EncryptAndSign),
            clientSettings4);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(clientSettings1 != clientSettings4);

        Trace.WriteInfo(TraceType, "server settings");

        SecuritySettings serverSettings1 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");
        error = serverSettings1.EnableClaimBasedAuthOnClients(L"c1=v1&&c2=v2||c3=v3", L"c0=v0");
        VERIFY_IS_TRUE(error.IsSuccess());

        SecuritySettings serverSettings2 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");
        error = serverSettings2.EnableClaimBasedAuthOnClients(L"c3=v3 || c2=V2 && C1=v1", L"c0=v0");
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(serverSettings1 == serverSettings2);

        SecuritySettings serverSettings3 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");
        error = serverSettings3.EnableClaimBasedAuthOnClients(L"c3=v3 || c2=v2", L"c0=v0");
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(serverSettings1 != serverSettings3);
        VERIFY_IS_TRUE(serverSettings2 != serverSettings3);

        SecuritySettings serverSettings4 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");
        error = serverSettings4.EnableClaimBasedAuthOnClients(L"c1=v1&&c2=v2", L"c0=v0");
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(serverSettings1 != serverSettings4);
        VERIFY_IS_TRUE(serverSettings2 != serverSettings4);
        VERIFY_IS_TRUE(serverSettings3 != serverSettings4);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(KerbEqualityTest)
    {
        ENTER;

        SecuritySettings settings1;
        auto error = SecuritySettings::CreateKerberos(
            L"WindowsFabric/a.b.c",
            L"redmond\\u1,redmond\\u2",
            wformatString(ProtectionLevel::EncryptAndSign),
            settings1);

        VERIFY_IS_TRUE(error.IsSuccess());

        SecuritySettings settings2;
        error = SecuritySettings::CreateKerberos(
            L"WindowsFabric/a.b.c",
            L"redmond\\U2,redmond\\u1",
            wformatString(ProtectionLevel::EncryptAndSign),
            settings2);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(settings1 == settings2);

        SecuritySettings settings3;
        error = SecuritySettings::CreateKerberos(
            L"WindowsFabric/a.b.c",
            L"redmond\\u2",
            wformatString(ProtectionLevel::EncryptAndSign),
            settings3);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(settings1 != settings3);

        SecuritySettings settings4;
        error = SecuritySettings::CreateKerberos(
            L"WindowsFabric/a.b.c",
            L"redmond\\u2,redmond\\u1,redmond\\u3",
            wformatString(ProtectionLevel::EncryptAndSign),
            settings4);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(settings1 != settings4);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(SslEqualityTest)
    {
        ENTER;

        SecuritySettings settings1 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabricRing.Rings.WinFabricTestDomain.com");

        SecuritySettings settings2 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabricRing.Rings.WinFabricTestDomain.com");

        VERIFY_IS_TRUE(settings1 == settings2);

        SecuritySettings settings3 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"WinFabricRing.Rings.WinFabricTestDomain.com,SomethingExtra");

        VERIFY_IS_TRUE(settings1 != settings3);

        SecuritySettings settings4 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"SomethingExtra,WinFabricRing.Rings.WinFabricTestDomain.com");

        VERIFY_IS_TRUE(settings3 == settings4);

        SecuritySettings settings5 = TTestUtil::CreateX509Settings(
            L"CN=WinFabric-Test-SAN1-Alice",
            L"SomethingExtra,WinFabricRing.Rings.WinFabricTestDomain.com,Extra2");

        VERIFY_IS_TRUE(settings3 != settings5);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(ClaimsTest)
    {
        ENTER;

        // basic test
        {
            SecuritySettings serverSecuritySettings0 = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");

            const wstring claimTypes[] = {L"Type0", L"Type1"};
            const wstring claimValues[] = {L"Value0", L"Value1"};

            auto error = serverSecuritySettings0.EnableClaimBasedAuthOnClients(
                claimTypes[0] + L"=" + claimValues[0],
                claimTypes[1] + L"=" + claimValues[1]);

            VERIFY_IS_TRUE(error.IsSuccess());

            auto serverSecuritySettings = serverSecuritySettings0;
            Trace.WriteInfo(TraceType, "serverSecuritySettings0={0}", serverSecuritySettings0);
            Trace.WriteInfo(TraceType, "serverSecuritySettings={0}", serverSecuritySettings);
            VERIFY_IS_TRUE(serverSecuritySettings == serverSecuritySettings0);

            SecuritySettings::RoleClaims rc0;
            rc0.AddClaim(L"type0", L"value0");
            SecuritySettings::RoleClaims rc1;
            rc1.AddClaim(L"type1", L"value1");

            VERIFY_IS_TRUE(serverSecuritySettings.AdminClientClaims().Value().size() == 1);

            VERIFY_IS_TRUE(!serverSecuritySettings.AdminClientClaims().Contains(rc0));
            VERIFY_IS_TRUE(serverSecuritySettings.AdminClientClaims().Contains(rc1));

            VERIFY_IS_TRUE(!rc0.IsInRole(serverSecuritySettings.AdminClientClaims()));
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.AdminClientClaims()));

            VERIFY_IS_TRUE(serverSecuritySettings.ClientClaims().Value().size() == 2);

            VERIFY_IS_TRUE(serverSecuritySettings.ClientClaims().Contains(rc0));
            VERIFY_IS_TRUE(serverSecuritySettings.ClientClaims().Contains(rc1));

            VERIFY_IS_TRUE(rc0.IsInRole(serverSecuritySettings.ClientClaims()));
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.ClientClaims()));

            SecuritySettings::RoleClaims empty;
            VERIFY_IS_TRUE(!empty.IsInRole(serverSecuritySettings.AdminClientClaims()));
            VERIFY_IS_TRUE(!empty.IsInRole(serverSecuritySettings.ClientClaims()));

            SecuritySettings::RoleClaims rc2;
            rc2.AddClaim(L"type3", L"value3");
            VERIFY_IS_TRUE(!rc2.IsInRole(serverSecuritySettings.AdminClientClaims()));
            VERIFY_IS_TRUE(!rc2.IsInRole(serverSecuritySettings.ClientClaims()));

            rc2.AddClaim(L"type0", L"value0");
            rc2.AddClaim(L"type1", L"value1");
            VERIFY_IS_TRUE(rc2.IsInRole(serverSecuritySettings.AdminClientClaims()));
            VERIFY_IS_TRUE(rc2.IsInRole(serverSecuritySettings.ClientClaims()));

            rc2.AddClaim(L"type2", L"value2");
            VERIFY_IS_TRUE(rc2.IsInRole(serverSecuritySettings.AdminClientClaims()));
            VERIFY_IS_TRUE(rc2.IsInRole(serverSecuritySettings.ClientClaims()));
        }

        // test combinations
        {
            SecuritySettings serverSecuritySettings = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");

            auto error = serverSecuritySettings.EnableClaimBasedAuthOnClients(
                L"t1 = v1&&t2=v2 && t3 = v3 || t4 = v4 || t2 = v2 && t1 = v1 && t2=v2",
                L"t5 = v5 || t6 = v6 && t7=v7 || t2=v2 && t3=v3 && t1=v1 && t4=v4 || t5 = vv || t6 = v6 && t5 = vv");

            VERIFY_IS_TRUE(error.IsSuccess());

            VERIFY_IS_TRUE(serverSecuritySettings.AdminClientClaims().Value().size() == 4);
            VERIFY_IS_TRUE(serverSecuritySettings.ClientClaims().Value().size() == 5);

            SecuritySettings::RoleClaims rc1;
            rc1.AddClaim(L"t2", L"V2");
            VERIFY_IS_TRUE(!rc1.IsInRole(serverSecuritySettings.ClientClaims()));

            rc1.AddClaim(L"t1", L"v1");
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.ClientClaims()));

            rc1.AddClaim(L"tx", L"vx");
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.ClientClaims()));
            VERIFY_IS_TRUE(!rc1.IsInRole(serverSecuritySettings.AdminClientClaims()));

            rc1.AddClaim(L"t3", L"v3");
            VERIFY_IS_TRUE(!rc1.IsInRole(serverSecuritySettings.AdminClientClaims()));

            rc1.AddClaim(L"t4", L"v4");
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.AdminClientClaims()));

            rc1.AddClaim(L"ty", L"vy");
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.AdminClientClaims()));

            SecuritySettings::RoleClaims rc2;
            rc2.AddClaim(L"t5", L"vv");
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.ClientClaims()));
            VERIFY_IS_TRUE(rc1.IsInRole(serverSecuritySettings.AdminClientClaims()));
        }

        // security provider negative test
        {
            SecuritySettings serverSecuritySettings;
            auto error = SecuritySettings::CreateNegotiateServer(L"", serverSecuritySettings);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(
                L"ClaimType1=ClaimValue1",
                L"ClaimType2=ClaimValue2");

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));
        }

        // negative tests with invalid claim strings
        {
            SecuritySettings serverSecuritySettings = TTestUtil::CreateX509Settings(L"CN=WinFabric-Test-SAN1-Alice");

            auto error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"||", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"||||", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"|| ||", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"&&", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"&&&&", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"&& &&", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"&&||", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L" && || ", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"||&&", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"|| &&", L"");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"t=", L"t1=v1");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"=v", L"t1=v1");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"=", L"t1=v1");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L" = ", L"t1=v1");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"= && = || =", L"t1=v1");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = serverSecuritySettings.EnableClaimBasedAuthOnClients(L"t= && =v || x=y", L"t1=v1");
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));
        }

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameDuplicate0)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "test duplicate names without issuer pinning");
        SecurityConfig::X509NameMap nameMap;
        auto name = L"testName";
        nameMap.Add(name);
        nameMap.Add(name);

        VERIFY_IS_TRUE(nameMap.Size() == 1);

        auto match = nameMap.Find(name);
        VERIFY_IS_TRUE(match != nameMap.CEnd());
        VERIFY_IS_TRUE(match->second.IsEmpty());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameDuplicate1)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "test duplicate names, one with issuer pinning, one without issuer");
        SecurityConfig::X509NameMap nameMap;
        auto name = L"testName";
        auto issuerCertThumbprint = L"99 88 77 66 55 44 33 22 11 00 aa bb cc dd ee ff 00 11 22 33";
        nameMap.Add(name, issuerCertThumbprint);
        nameMap.Add(name);

        VERIFY_IS_TRUE(nameMap.Size() == 1);

        auto match = nameMap.Find(name);
        VERIFY_IS_TRUE(match != nameMap.CEnd());
        VERIFY_IS_TRUE(match->second.IsEmpty());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameDuplicate2)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "test duplicate names, one with issuer pinning, one without");

        SecurityConfig::X509NameMap nameMap;
        auto name = L"testName";
        nameMap.Add(name);
        auto issuerCertThumbprint = L"99 88 77 66 55 44 33 22 11 00 aa bb cc dd ee ff 00 11 22 33";
        nameMap.Add(name, issuerCertThumbprint);

        VERIFY_IS_TRUE(nameMap.Size() == 1);

        auto match = nameMap.Find(name);
        VERIFY_IS_TRUE(match != nameMap.CEnd());
        VERIFY_IS_TRUE(match->second.IsEmpty());

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameDuplicate3)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "test duplicate names, both with the same issuer pinning");

        SecurityConfig::X509NameMap nameMap;
        auto name = L"testName";
        auto issuerCertThumbprint = L"99 88 77 66 55 44 33 22 11 00 aa bb cc dd ee ff 00 11 22 33";
        nameMap.Add(name, issuerCertThumbprint);
        nameMap.Add(name, issuerCertThumbprint);

        VERIFY_IS_TRUE(nameMap.Size() == 1);

        auto match = nameMap.Find(name);
        VERIFY_IS_TRUE(match != nameMap.CEnd());
        VERIFY_IS_TRUE(match->second.Size() == 1);

        Thumbprint::SPtr thumbprint;
        auto error = Thumbprint::Create(issuerCertThumbprint, thumbprint);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(match->second.Contains(thumbprint));

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(X509NameDuplicate4)
    {
        ENTER;

        Trace.WriteInfo(TraceType, "test duplicate names, both with different issuer pinning");

        SecurityConfig::X509NameMap nameMap;
        auto name = L"testName";
        auto issuerCertThumbprint1 = L"99 88 77 66 55 44 33 22 11 00 aa bb cc dd ee ff 00 11 22 33";
        nameMap.Add(name, issuerCertThumbprint1);

        auto issuerCertThumbprint2 = L"aa 88 77 66 55 44 33 22 11 00 aa bb cc dd ee ff 00 11 22 bb";
        nameMap.Add(name, issuerCertThumbprint2);

        VERIFY_IS_TRUE(nameMap.Size() == 1);

        auto match = nameMap.Find(name);
        VERIFY_IS_TRUE(match != nameMap.CEnd());
        VERIFY_IS_TRUE(match->second.Size() == 2);

        Thumbprint::SPtr thumbprint1;
        auto error = Thumbprint::Create(issuerCertThumbprint1, thumbprint1);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(match->second.Contains(thumbprint1));

        Thumbprint::SPtr thumbprint2;
        error = Thumbprint::Create(issuerCertThumbprint2, thumbprint2);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(match->second.Contains(thumbprint2));

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()

    void SecuritySettingsTest::RoundTripTest(
        wstring const & certIssuerThumbprints,
        wstring const & remoteCertThumbprnits,
        wstring const & findValueSecondary,
        X509FindType::Enum findType)
    {
        {
            SecuritySettings fromConfig = TTestUtil::CreateX509SettingsForRoundTripTest(
                certIssuerThumbprints,
                remoteCertThumbprnits,
                findValueSecondary,
                findType);

            Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);

            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            fromConfig.ToPublicApi(heap, publicType);

            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig, fromPublicType);
        }

        {
            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            {
                // create fromConfig in inner scope to detect missing memory allocation in ToPublicApi
                SecuritySettings fromConfig = TTestUtil::CreateX509SettingsForRoundTripTest(
                    certIssuerThumbprints,
                    remoteCertThumbprnits,
                    findValueSecondary,
                    findType);

                Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);
                fromConfig.ToPublicApi(heap, publicType);
            }

            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            SecuritySettings fromConfig2 = TTestUtil::CreateX509SettingsForRoundTripTest(
                certIssuerThumbprints,
                remoteCertThumbprnits,
                findValueSecondary,
                findType);

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig2, fromPublicType);
        }

        {
            FABRIC_SECURITY_CREDENTIALS publicType = {};
            ScopedHeap heap;
            SecuritySettings fromConfig2;
            {
                // create fromConfig in inner scope to detect missing memory allocation in ToPublicApi
                SecuritySettings fromConfig = TTestUtil::CreateX509SettingsForRoundTripTest(
                    certIssuerThumbprints,
                    remoteCertThumbprnits,
                    findValueSecondary,
                    findType);

                fromConfig2 = fromConfig;

                Trace.WriteInfo(TraceType, "FromConfiguration={0}", fromConfig);
                fromConfig.ToPublicApi(heap, publicType);
            }

            SecuritySettings fromPublicType;
            auto error = SecuritySettings::FromPublicApi(publicType, fromPublicType);
            VERIFY_IS_TRUE(error.IsSuccess());

            Trace.WriteInfo(TraceType, "FromPublicApi={0}", fromPublicType);
            VERIFY_ARE_EQUAL2(fromConfig2, fromPublicType);
        }
    }
}
