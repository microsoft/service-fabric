// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <cstdlib>

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    StringLiteral const TestSource("NamingUriTest");

    class NamingUriTest
    {
    protected:
        void UriTestHelper(wstring const & filename, bool expectedSuccess);
        static void CheckIsNotValidNamingUri(wstring const & path);
        static void EscapeUnescapeTest(wstring const &originalUri);
    };

    BOOST_FIXTURE_TEST_SUITE(NamingUriTestSuite,NamingUriTest)

    BOOST_AUTO_TEST_CASE(NamingUriCreation)
    {
        NamingUri name1(L"Iris/Hydra");
        NamingUri name2(L"Iris/Awakening");
        NamingUri name3(L"Lunatica/New_Shores");
        NamingUri name4(L"Lunatica");

        VERIFY_IS_TRUE(name1.Authority == name2.Authority);
        VERIFY_IS_TRUE(name3.Authority == name4.Authority);
        VERIFY_IS_FALSE(name1 == name3);
        VERIFY_IS_FALSE(name1 == name2);
        NamingUri nameCopy = name1;
        VERIFY_IS_TRUE(nameCopy == name1);
    }

    BOOST_AUTO_TEST_CASE(NamingUriToString)
    {
        NamingUri name1(L"Nightwish/Oceanborn");
        NamingUri name2(NamingUri::RootAuthority);
        NamingUri name3(L"Nightwish");
        std::wstring text1(L"fabric:/Nightwish/Oceanborn");
        std::wstring text2(L"fabric:");
        std::wstring text3(L"fabric:/Nightwish");
        VERIFY_IS_TRUE(name1.ToString() == text1);
        VERIFY_IS_TRUE(name2.ToString() == text2);
        VERIFY_IS_TRUE(name3.ToString() == text3);
        VERIFY_IS_FALSE(name1.ToString() == text2);
        VERIFY_IS_FALSE(name1.ToString() == text3);
        VERIFY_IS_FALSE(name2.ToString() == text3);

        NamingUri ts;
        VERIFY_IS_TRUE(ts.TryParse(L"fabric:/alexisonfire", ts));
        VERIFY_ARE_EQUAL2(L"fabric:/alexisonfire", ts.ToString());
    }

    BOOST_AUTO_TEST_CASE(NamingUriParse)
    {
        std::wstring text1(L"fabric:/apocalyptica/Reflections");
        std::wstring text2(L"fabric:/apocalyptica");
        std::wstring text3(L"fabric:");
        std::wstring text4(L"http://msdn.microsoft.com");
        std::wstring text5(L"fabric:/apocalyptica/Reflections/");
        std::wstring text6(L"fabric://authority");
        std::wstring text1p(L"fabric:/apocalyptica/Reflections");
        std::wstring text2p(L"fabric:/apocalyptica");
        std::wstring text3p(L"fabric:");

        NamingUri temp(NamingUri::RootAuthority);

        VERIFY_IS_TRUE(NamingUri::TryParse(text1, temp));
        VERIFY_IS_TRUE(temp.ToString() == text1p);
        VERIFY_IS_TRUE(NamingUri::TryParse(text2, temp));
        VERIFY_IS_TRUE(temp.ToString() == text2p);
        VERIFY_IS_TRUE(NamingUri::TryParse(text3, temp));
        VERIFY_IS_TRUE(temp.ToString() == text3p);

        temp = NamingUri(NamingUri::RootAuthority);
        VERIFY_IS_TRUE(temp.ToString() == L"fabric:");

        VERIFY_IS_FALSE(NamingUri::TryParse(text4, temp));
        VERIFY_IS_TRUE(temp.ToString() == L"fabric:");
        VERIFY_IS_FALSE(NamingUri::TryParse(text5, temp));
        VERIFY_IS_TRUE(temp.ToString() == L"fabric:");
        VERIFY_IS_FALSE(NamingUri::TryParse(text6, temp));
        VERIFY_IS_TRUE(temp.ToString() == L"fabric:");
    }

    BOOST_AUTO_TEST_CASE(IsNamingUri)
    {
        VERIFY_IS_TRUE(NamingUri::IsNamingUri(NamingUri()));
        VERIFY_IS_TRUE(NamingUri::IsNamingUri(Uri(L"fabric", L"", L"a")));
        VERIFY_IS_TRUE(NamingUri::IsNamingUri(Uri(L"fabric", L"", L"a/b")));
        VERIFY_IS_TRUE(NamingUri::IsNamingUri(Uri(L"fabric", L"", L"a/b#c")));
        VERIFY_IS_TRUE(NamingUri::IsNamingUri(Uri(L"fabric", L"", L"a/b{c}")));
        VERIFY_IS_FALSE(NamingUri::IsNamingUri(Uri(L"fabric", L"x", L"a")));
        VERIFY_IS_FALSE(NamingUri::IsNamingUri(Uri(L"ns", L"x", L"a")));
    }

    BOOST_AUTO_TEST_CASE(RootName)
    {
        NamingUri one(L"one");
        NamingUri two(L"one/two");
        NamingUri three(L"one/three");
        NamingUri four(L"four");
        NamingUri five(L"four/five");

        VERIFY_IS_TRUE(one.GetAuthorityName() == two.GetAuthorityName());
        VERIFY_IS_TRUE(one.GetAuthorityName() == three.GetAuthorityName());
        VERIFY_IS_FALSE(one.GetAuthorityName() == four.GetAuthorityName());
        VERIFY_IS_FALSE(one.GetAuthorityName() == five.GetAuthorityName());

        VERIFY_IS_FALSE(five.GetAuthorityName() == two.GetAuthorityName());
        VERIFY_IS_FALSE(five.GetAuthorityName() == three.GetAuthorityName());
        VERIFY_IS_TRUE(five.GetAuthorityName() == four.GetAuthorityName());
        VERIFY_IS_TRUE(five.GetAuthorityName() == five.GetAuthorityName());
    }

    BOOST_AUTO_TEST_CASE(ParentName)
    {
        NamingUri one(L"Nickelback");
        NamingUri oneA(L"Nickelback/a");
        NamingUri oneB(L"Nickelback/a/b");
        NamingUri oneC(L"Nickelback/a/b/c");

        VERIFY_IS_TRUE(oneC.GetParentName() == oneB);
        VERIFY_IS_TRUE(oneB.GetParentName() == oneA);
        VERIFY_IS_TRUE(oneA.GetParentName() == one);
        VERIFY_IS_TRUE(one.GetParentName() == NamingUri::RootNamingUri);
    }

    BOOST_AUTO_TEST_CASE(IsPrefixOf)
    {
        NamingUri empty;
        NamingUri a;
        NamingUri aB;
        NamingUri ab;
        NamingUri ab_c;
        NamingUri ab_cD;
        NamingUri ab_cd;
        NamingUri ab_cd_e;
        NamingUri ab_cd_eF;
        NamingUri ab_cd_ef;
        NamingUri ab_cd_ef_0;
        NamingUri ab_cd_ef_01;

        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:", empty));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/a", a));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/aB", aB));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab", ab));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/c", ab_c));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cD", ab_cD));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cd", ab_cd));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cd/e", ab_cd_e));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cd/eF", ab_cd_eF));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cd/ef", ab_cd_ef));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cd/ef#0", ab_cd_ef_0));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/ab/cd/ef#01", ab_cd_ef_01));

        // Authority
        VERIFY_IS_FALSE(a.IsPrefixOf(a));
        VERIFY_IS_FALSE(a.IsPrefixOf(aB));
        VERIFY_IS_FALSE(a.IsPrefixOf(ab));
        VERIFY_IS_FALSE(a.IsPrefixOf(ab_c));
        VERIFY_IS_FALSE(a.IsPrefixOf(ab_cd_e));
        VERIFY_IS_FALSE(a.IsPrefixOf(ab_cd_ef_0));

        VERIFY_IS_FALSE(aB.IsPrefixOf(a));
        VERIFY_IS_FALSE(aB.IsPrefixOf(aB));
        VERIFY_IS_FALSE(aB.IsPrefixOf(ab));
        VERIFY_IS_FALSE(aB.IsPrefixOf(ab_c)); // case-sensitive authority
        VERIFY_IS_FALSE(aB.IsPrefixOf(ab_cD)); // case-sensitive authority
        VERIFY_IS_FALSE(aB.IsPrefixOf(ab_cd_e)); // case-sensitive authority
        VERIFY_IS_FALSE(aB.IsPrefixOf(ab_cd_eF)); // case-sensitive authority
        VERIFY_IS_FALSE(aB.IsPrefixOf(ab_cd_ef_0)); // case-sensitive authority

        VERIFY_IS_TRUE(ab.IsPrefixOf(ab_c));
        VERIFY_IS_TRUE(ab.IsPrefixOf(ab_cD));
        VERIFY_IS_TRUE(ab.IsPrefixOf(ab_cd_e));
        VERIFY_IS_TRUE(ab.IsPrefixOf(ab_cd_eF));
        VERIFY_IS_TRUE(ab.IsPrefixOf(ab_cd_ef_0));

        // One segment
        VERIFY_IS_FALSE(ab_c.IsPrefixOf(ab));
        VERIFY_IS_FALSE(ab_c.IsPrefixOf(ab_c));
        VERIFY_IS_FALSE(ab_c.IsPrefixOf(ab_cD));
        VERIFY_IS_FALSE(ab_c.IsPrefixOf(ab_cd_e));
        VERIFY_IS_FALSE(ab_c.IsPrefixOf(ab_cd_eF));
        VERIFY_IS_FALSE(ab_c.IsPrefixOf(ab_cd_ef_0));

        VERIFY_IS_FALSE(ab_cD.IsPrefixOf(ab));
        VERIFY_IS_FALSE(ab_cD.IsPrefixOf(ab_c));
        VERIFY_IS_FALSE(ab_cD.IsPrefixOf(ab_cD));
        VERIFY_IS_FALSE(ab_cD.IsPrefixOf(ab_cd_e));
        VERIFY_IS_FALSE(ab_cD.IsPrefixOf(ab_cd_eF));
        VERIFY_IS_FALSE(ab_cD.IsPrefixOf(ab_cd_ef_0));

        VERIFY_IS_TRUE(ab_cd.IsPrefixOf(ab_cd_e));
        VERIFY_IS_TRUE(ab_cd.IsPrefixOf(ab_cd_eF));
        VERIFY_IS_TRUE(ab_cd.IsPrefixOf(ab_cd_ef_0));

        // Two segments
        VERIFY_IS_FALSE(ab_cd_e.IsPrefixOf(ab_cd));
        VERIFY_IS_FALSE(ab_cd_e.IsPrefixOf(ab_cd_e));
        VERIFY_IS_FALSE(ab_cd_e.IsPrefixOf(ab_cd_eF));
        VERIFY_IS_FALSE(ab_cd_e.IsPrefixOf(ab_cd_ef_0));

        VERIFY_IS_FALSE(ab_cd_eF.IsPrefixOf(ab_cd));
        VERIFY_IS_FALSE(ab_cd_eF.IsPrefixOf(ab_cd_e));
        VERIFY_IS_FALSE(ab_cd_eF.IsPrefixOf(ab_cd_eF));
        VERIFY_IS_FALSE(ab_cd_eF.IsPrefixOf(ab_cd_ef_0));

        VERIFY_IS_TRUE(ab_cd_ef.IsPrefixOf(ab_cd_ef_0));

        // Fragment
        VERIFY_IS_FALSE(ab_cd_ef_0.IsPrefixOf(ab_cd_ef));
        VERIFY_IS_FALSE(ab_cd_ef_0.IsPrefixOf(ab_cd_ef_0));
        VERIFY_IS_FALSE(ab_cd_ef_0.IsPrefixOf(ab_cd_ef_01));
    }

    BOOST_AUTO_TEST_CASE(InvalidCharacterTest)
    {
        NamingUri uri;
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/Ara\bKus", uri));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/SunnO)))", uri));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan", uri));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan", uri));
        VERIFY_ARE_EQUAL2(L"fabric:/The_Dillinger_Escape_Plan", uri.ToString()); // case-sensitive authority
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Calculating_Infinity", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Calculating_Infinity/", uri));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Calculating_Infinity/*#..", uri));
        VERIFY_IS_TRUE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Calculating_Infinity/Jim_Fear", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Calculating_Infinity/Jim_Fear/", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan//Miss_Machine", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan///Miss_Machine", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Miss_Machine//Unretrofied", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:/The_Dillinger_Escape_Plan/Miss_Machine/Unretrofied//", uri));
        // Cannot have subnames at the root authority
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:///The_Dillinger_Escape_Plan", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:///The_Dillinger_Escape_Plan/Ire_Works", uri));
        VERIFY_IS_FALSE(NamingUri::TryParse(L"fabric:////The_Dillinger_Escape_Plan", uri));

        for (auto it = NamingUri::ReservedTokenDelimiters->begin(); it != NamingUri::ReservedTokenDelimiters->end(); ++it)
        {
            CheckIsNotValidNamingUri(wformatString("a{0}", *it));
            CheckIsNotValidNamingUri(wformatString("a/b{0}c", *it));
        }
    }

    BOOST_AUTO_TEST_CASE(UnsupportedUriTest)
    {
        Trace.WriteInfo(TestSource, "UnsupportedUriTest");

        UriTestHelper(L"UnsupportedUris.txt", false);
    }

    BOOST_AUTO_TEST_CASE(SupportedUriTest)
    {
        Trace.WriteInfo(TestSource, "SupportedUriTest");

        UriTestHelper(L"SupportedUris.txt", true);
    }

    BOOST_AUTO_TEST_CASE(EscapeTest)
    {
        wstring input;
        wstring result1;
        wstring result2;

        input = L"host`~!@$^&*()-_=+[{]}\\|;:'\",<.> /path`~!@$^&*()-_=+[{]}\\|;:'\",<.> ";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"host%60~!@$%5E%26*()-_=+%5B%7B%5D%7D%5C%7C;:'%22,%3C.%3E%20/path%60~!@$%5E%26*()-_=+%5B%7B%5D%7D%5C%7C;:'%22,%3C.%3E%20");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"host space/path space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"host%20space/path%20space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"host space/path space?query space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"host%20space/path%20space?query space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"host space/path space#fragment space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"host%20space/path%20space#fragment space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"host space/path space?query space#fragment space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"host%20space/path%20space?query space#fragment space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http:/host space/path space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http:/host%20space/path%20space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host space/path space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host space/path%20space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http:///host space/path space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http:///host%20space/path%20space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host/{path}";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host/%7Bpath%7D");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host/123&456";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host/123%26456");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host/[12.3,45.6]";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host/%5B12.3,45.6%5D");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host/123 456";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host/123%20456");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host space/path1 space/path2 space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host space/path1%20space/path2%20space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host space/path1 space/path2 space?query space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host space/path1%20space/path2%20space?query space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host space/path1 space/path2 space#segment space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host space/path1%20space/path2%20space#segment space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host space/path1 space/path2 space?query space#segment space";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host space/path1%20space/path2%20space?query space#segment space");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);

        input = L"http://host@[]/path1@[]/path2@[]?query@[]#segment@[]";
        NamingUri::EscapeString(input, result1);
        VERIFY_ARE_EQUAL(result1, L"http://host@[]/path1@%5B%5D/path2@%5B%5D?query@[]#segment@[]");
        NamingUri::UnescapeString(result1, result2);
        VERIFY_ARE_EQUAL(result2, input);
    }

    BOOST_AUTO_TEST_CASE(UrlEscapeTest)
    {
        wstring input;
        wstring expectedEncoded;
        wstring encodingResult;
        wstring decodingResult;

        input = L"$---+---,---:---;---=---@";
        expectedEncoded = L"%24---%2B---%2C---%3A---%3B---%3D---%40";
        NamingUri::UrlEscapeString(input, encodingResult);
        VERIFY_ARE_EQUAL(encodingResult, expectedEncoded);
        NamingUri::UnescapeString(expectedEncoded, decodingResult);
        VERIFY_ARE_EQUAL(decodingResult, input);

        input = L"&---/---\\---?---#---<--->---[---]---{---}---|---^";
        expectedEncoded = L"%26---%2F---%5C---%3F---%23---%3C---%3E---%5B---%5D---%7B---%7D---%7C---%5E";
        NamingUri::UrlEscapeString(input, encodingResult);
        VERIFY_ARE_EQUAL(encodingResult, expectedEncoded);
        NamingUri::UnescapeString(expectedEncoded, decodingResult);
        VERIFY_ARE_EQUAL(decodingResult, input);

        input = L"None-of-this-should-be-encoded";
        expectedEncoded = L"None-of-this-should-be-encoded";
        NamingUri::UrlEscapeString(input, encodingResult);
        VERIFY_ARE_EQUAL(encodingResult, expectedEncoded);
        NamingUri::UnescapeString(expectedEncoded, decodingResult);
        VERIFY_ARE_EQUAL(decodingResult, input);

        input = L"$";
        expectedEncoded = L"%24";
        NamingUri::UrlEscapeString(input, encodingResult);
        VERIFY_ARE_EQUAL(encodingResult, expectedEncoded);
        NamingUri::UnescapeString(expectedEncoded, decodingResult);
        VERIFY_ARE_EQUAL(decodingResult, input);

        // Do a check to make sure that the EscapeString method on which UrlEscapeString is based is actually called
        input = L" ";
        expectedEncoded = L"%20";
        NamingUri::UrlEscapeString(input, encodingResult);
        VERIFY_ARE_EQUAL(encodingResult, expectedEncoded);
        NamingUri::UnescapeString(expectedEncoded, decodingResult);
        VERIFY_ARE_EQUAL(decodingResult, input);
    }

    BOOST_AUTO_TEST_CASE(NamingUriCannonicalizationTest)
    {
        Trace.WriteInfo(TestSource, "NamingUriCannonicalizationTest");

        // These URI's must be escaped
// LINUXTODO
#if !defined(PLATFORM_UNIX)
        NamingUriTest::EscapeUnescapeTest(L"fabric:/اعددالكلماتعددالويبمثيلخدمة");
#endif
        NamingUriTest::EscapeUnescapeTest(L"fabric:/My app with space");
    }

    BOOST_AUTO_TEST_CASE(NamingUriTryCombine)
    {
        Trace.WriteInfo(TestSource, "*** NamingUriTryCombine");

        NamingUri result;
        NamingUri prefix(L"a");

        VERIFY_IS_TRUE(prefix.TryCombine(L"", result));
        VERIFY_ARE_EQUAL(result, NamingUri(L"a"));

        VERIFY_IS_TRUE(result.TryCombine(L"b", result));
        VERIFY_ARE_EQUAL(result, NamingUri(L"a/b"));

        VERIFY_IS_TRUE(result.TryCombine(L"", result));
        VERIFY_ARE_EQUAL(result, NamingUri(L"a/b"));

        VERIFY_IS_TRUE(result.TryCombine(L"c", result));
        VERIFY_ARE_EQUAL(result, NamingUri(L"a/b/c"));

        VERIFY_IS_FALSE(result.TryCombine(L"/", result));
    }

    BOOST_AUTO_TEST_SUITE_END()

    void NamingUriTest::UriTestHelper(wstring const & filename, bool expectedSuccess)
    {
        wstring srcDirectory = Directory::GetCurrentDirectory();
	    wstring bins;
        if (Environment::GetEnvironmentVariableW(L"_NTTREE", bins, Common::NOTHROW()))
        {
            srcDirectory = Path::Combine(bins, L"FabricUnitTests");
        }
	    // RunCITs will end up setting current directory to FabricUnitTests\log
	    else if (StringUtility::EndsWith<wstring>(srcDirectory, L"log"))
        {
            srcDirectory = Path::Combine(srcDirectory, L"..\\");
        }

        File file;
        auto error = file.TryOpen(Path::Combine(srcDirectory, filename));
        VERIFY_IS_TRUE(error.IsSuccess());

        auto fileSize = file.size();

        vector<char> buffer(static_cast<size_t>(fileSize));

        DWORD bytesRead;

        error = file.TryRead2(reinterpret_cast<void*>(buffer.data()), static_cast<int>(fileSize), bytesRead);

        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(static_cast<int64>(bytesRead) == fileSize);

        error = file.Close2();

        VERIFY_IS_TRUE(error.IsSuccess());

        vector<wchar_t> wbuffer(buffer.size());

        int written = MultiByteToWideChar(
          CP_UTF8,
          MB_ERR_INVALID_CHARS,
          buffer.data(),
          static_cast<int>(buffer.size()),
          wbuffer.data(),
          static_cast<int>(wbuffer.size()));

        VERIFY_IS_TRUE(static_cast<size_t>(written) == buffer.size());

        wbuffer.push_back(0);
        vector<wstring> namesToParse;

        StringUtility::Split<wstring>(wstring(wbuffer.data()), namesToParse, L"\n\r");

        vector<wstring> failedNames;

        for (auto iter = namesToParse.begin(); iter != namesToParse.end(); ++iter)
        {
            wstring const & toParse = *iter;

            // skip comments
            if (StringUtility::StartsWith<wstring>(toParse, L"#"))
            {
                continue;
            }

            NamingUri parsedName;

            bool success = NamingUri::TryParse(toParse, parsedName);
            if (success == expectedSuccess)
            {
                Trace.WriteInfo(TestSource, "{0}", toParse);
            }
            else
            {
                Trace.WriteWarning(TestSource, "{0}", toParse);
                failedNames.push_back(toParse);
            }
        }

        Trace.WriteInfo(TestSource, "Failed names count = {0}", failedNames.size());

        VERIFY_IS_TRUE(failedNames.empty());
    }

    void NamingUriTest::CheckIsNotValidNamingUri(wstring const & path)
    {
        Trace.WriteNoise(TestSource, "CheckIsNotValidNamingUri {0}", path);
        VERIFY_IS_FALSE(NamingUri::IsNamingUri(Uri(L"fabric:", L"", path)));

        wstring name = wformatString("fabric:/{0}", path);
        Trace.WriteNoise(TestSource, "CheckIsNotValidNamingUri {0}", name);
        NamingUri uri;
        VERIFY_IS_FALSE(NamingUri::TryParse(name, uri));
    }

    void NamingUriTest::EscapeUnescapeTest(wstring const &originalUri)
    {
        wstring escapedUri;

        Trace.WriteInfo(TestSource, "EscapeUnescapeTest for {0}", originalUri);

        auto errorCode = NamingUri::EscapeString(originalUri, escapedUri);
        VERIFY_IS_TRUE(errorCode.IsSuccess());

        // URI must have been escaped, so confirm that it is not the same as the original uri.
        VERIFY_IS_FALSE(StringUtility::AreEqualCaseInsensitive(originalUri, escapedUri));

        wstring unescapedUri;
        errorCode = NamingUri::UnescapeString(escapedUri, unescapedUri);
        VERIFY_IS_TRUE(errorCode.IsSuccess());
        VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(originalUri, unescapedUri));
    }
}
