// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/StringUtility.h"

using namespace Common;
using namespace std;

BOOST_AUTO_TEST_SUITE2(StringUtilityTests)


BOOST_AUTO_TEST_CASE(TrimExtraCharacters)
{
    wstring trimmedString1 = StringUtility::LimitNumberOfCharacters(wstring(L"2+3-4+5-6+7"), L'+', 2);
    VERIFY_IS_TRUE(trimmedString1 == wstring(L"2+3-4+5-6"));

    wstring trimmedString2 = StringUtility::LimitNumberOfCharacters(wstring(L"aabb"), L'b', 0);
    VERIFY_IS_TRUE(trimmedString2 == wstring(L"aa"));

    wstring trimmedString3 = StringUtility::LimitNumberOfCharacters(wstring(L"babb"), L'b', 0);
    VERIFY_IS_TRUE(trimmedString3 == wstring(L""));

    wstring trimmedString4 = StringUtility::LimitNumberOfCharacters(wstring(L"bacb"), L'c', 1);
    VERIFY_IS_TRUE(trimmedString4 == wstring(L"bacb"));
}

BOOST_AUTO_TEST_CASE(RemoveDotAndGetDouble)
{
    wstring trimmedString1 = StringUtility::LimitNumberOfCharacters(wstring(L"17.03.1-ee-3"), L'.', 1);
    double trimmedDouble1 = 0.0;
    VERIFY_IS_TRUE(trimmedString1 == wstring(L"17.03"));
    VERIFY_IS_TRUE(StringUtility::TryFromWString<double>(trimmedString1, trimmedDouble1));
    VERIFY_IS_TRUE(trimmedDouble1 == 17.03);

    wstring trimmedString2 = StringUtility::LimitNumberOfCharacters(wstring(L"17.06.2-ce-4"), L'.', 1);
    double trimmedDouble2 = 0.0;
    VERIFY_IS_TRUE(trimmedString2 == wstring(L"17.06"));
    VERIFY_IS_TRUE(StringUtility::TryFromWString<double>(trimmedString2, trimmedDouble2));
    VERIFY_IS_TRUE(trimmedDouble2 == 17.06);
}

BOOST_AUTO_TEST_CASE(SmokeAreEqualCaseInsensitive)
{
    VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(L"Test UPPER/lower", L"TEST upper/LOWER"));
    VERIFY_IS_FALSE(StringUtility::AreEqualCaseInsensitive(L"aaa", L" Aaa"));
    VERIFY_IS_FALSE(StringUtility::AreEqualCaseInsensitive(L"aaa", L"aAb"));
}

BOOST_AUTO_TEST_CASE(SmokeAreEqualPrefixPartCaseInsensitive)
{
    VERIFY_IS_TRUE(StringUtility::AreEqualPrefixPartCaseInsensitive(wstring(L"servicefabric:/test"), wstring(L"ServiceFabric:/test"), L':'));
    VERIFY_IS_TRUE(StringUtility::AreEqualPrefixPartCaseInsensitive(wstring(L"servicefabric:/test"), wstring(L"ServiceFabric:/test"), L':'));
    VERIFY_IS_TRUE(StringUtility::AreEqualPrefixPartCaseInsensitive(wstring(L"serviceFabric:/test"), wstring(L"ServiceFabric:/test"), L':'));
    VERIFY_IS_TRUE(StringUtility::AreEqualPrefixPartCaseInsensitive(string("Random1_Run"), string("random1_Run"), '_'));
    VERIFY_IS_TRUE(StringUtility::AreEqualPrefixPartCaseInsensitive(string("Random1:Run"), string("random1:Run"), '_'));
    VERIFY_IS_FALSE(StringUtility::AreEqualPrefixPartCaseInsensitive(wstring(L"random1"), wstring(L"random2"), L','));
    VERIFY_IS_FALSE(StringUtility::AreEqualPrefixPartCaseInsensitive(wstring(L"random1:x"), wstring(L"random1:y"), L':'));
    VERIFY_IS_FALSE(StringUtility::AreEqualPrefixPartCaseInsensitive(string("random1:"), string("random1:y"), ':'));
    VERIFY_IS_FALSE(StringUtility::AreEqualPrefixPartCaseInsensitive(string("random1:Run"), string("random1:run"), ':'));
    VERIFY_IS_FALSE(StringUtility::AreEqualPrefixPartCaseInsensitive(string("Random1:Run"), string("random1:run"), ':'));
}

BOOST_AUTO_TEST_CASE(SmokeIsLessCaseInsensitive)
{
    VERIFY_IS_TRUE(StringUtility::IsLessCaseInsensitive(L"zzza", L"zzzb"));
    VERIFY_IS_FALSE(StringUtility::IsLessCaseInsensitive(L"denial", L"deniAl"));
    VERIFY_IS_TRUE(StringUtility::IsLessCaseInsensitive(L"mArket", L"mbA"));
    VERIFY_IS_FALSE(StringUtility::IsLessCaseInsensitive(L"cast", L"Bast"));
    VERIFY_IS_TRUE(StringUtility::IsLessCaseInsensitive(L"b", L"C"));
    VERIFY_IS_TRUE(StringUtility::IsLessCaseInsensitive(L"alt", L"Blt"));
}

BOOST_AUTO_TEST_CASE(SmokeContains1)
{
    string big = "Section one-two";
    VERIFY_IS_TRUE(StringUtility::Contains(big, string("one")));
    VERIFY_IS_TRUE(StringUtility::ContainsCaseInsensitive(big, string("OnE")));
    VERIFY_IS_FALSE(StringUtility::Contains(big, string("OnE")));
}

BOOST_AUTO_TEST_CASE(SmokeContains2)
{
    wstring big = L"Section one-two";
    VERIFY_IS_TRUE(StringUtility::Contains(big, wstring(L"one")));
    VERIFY_IS_TRUE(StringUtility::ContainsCaseInsensitive(big, wstring(L"OnE")));
    VERIFY_IS_FALSE(StringUtility::Contains(big, wstring(L"OnE")));
}

BOOST_AUTO_TEST_CASE(SmokeSplit1)
{
    wstring input = L" a,,  , string  with,    spaces and ,";
    vector<wstring> tokens;
    StringUtility::Split<wstring>(input, tokens, L" ");

    vector<wstring> vect;
    vect.push_back(L"a,,");
    vect.push_back(L",");
    vect.push_back(L"string");
    vect.push_back(L"with,");
    vect.push_back(L"spaces");
    vect.push_back(L"and");
    vect.push_back(L",");
            
    VERIFY_IS_TRUE(tokens == vect);
}

BOOST_AUTO_TEST_CASE(SmokeSplit2)
{
    wstring input = L" a,,  , string  with,    spaces and ,";
    vector<wstring> tokens;
    StringUtility::Split<wstring>(input, tokens, L",");
            
    vector<wstring> vect;
    vect.push_back(L" a");
    vect.push_back(L"  ");
    vect.push_back(L" string  with");
    vect.push_back(L"    spaces and ");
            
    VERIFY_IS_TRUE(tokens == vect);
}

BOOST_AUTO_TEST_CASE(SmokeSplit3)
{
    wstring input = L" a,,  , string  with,    spaces and ,";
    vector<wstring> tokens;
    StringUtility::Split<wstring>(input, tokens, L", ");

    vector<wstring> vect;
    vect.push_back(L"a");
    vect.push_back(L"string");
    vect.push_back(L"with");
    vect.push_back(L"spaces");
    vect.push_back(L"and");
            
    VERIFY_IS_TRUE(tokens == vect);
}

BOOST_AUTO_TEST_CASE(SmokeSplit4)
{
    wstring input = L" a string without any separators";
    vector<wstring> tokens;
    StringUtility::Split<wstring>(input, tokens, L";");
            
    vector<wstring> vect;
    vect.push_back(L" a string without any separators");
            
    VERIFY_IS_TRUE(tokens == vect);
}

BOOST_AUTO_TEST_CASE(SmokeSplit5)
{
    wstring input = L" a string without any separators";
    wstring token1;
    wstring token2;
    StringUtility::SplitOnce<wstring,wchar_t>(input, token1, token2, L';');
            
    VERIFY_IS_TRUE(token1 == input);
    VERIFY_IS_TRUE(token2 == L"");
}


BOOST_AUTO_TEST_CASE(SmokeSplit6)
{
    wstring input = L" a string without any separators";
    wstring token1;
    wstring token2;
    StringUtility::SplitOnce<wstring,wchar_t>(input, token1, token2, L':');
            
    VERIFY_IS_TRUE(token1 == input);
    VERIFY_IS_TRUE(token2 == L"");
}

BOOST_AUTO_TEST_CASE(SmokeSplit7)
{
    wstring input = L"HeaderName: HeaderValue";
    wstring token1;
    wstring token2;
    StringUtility::SplitOnce<wstring,wchar_t>(input, token1, token2, L':');
            
    VERIFY_IS_TRUE(token1 == L"HeaderName");
    VERIFY_IS_TRUE(token2 == L" HeaderValue");
}


BOOST_AUTO_TEST_CASE(SmokeSplit8)
{
    wstring input = L"SOAPAction: \"http://tempuri.org/samples\"";
    wstring token1;
    wstring token2;
    StringUtility::SplitOnce<wstring,wchar_t>(input, token1, token2, L':');
            
    VERIFY_IS_TRUE(token1 == L"SOAPAction");
    VERIFY_IS_TRUE(token2 == L" \"http://tempuri.org/samples\"");
}
        
BOOST_AUTO_TEST_CASE(SmokeTrimWhitespace1)
{
    wstring input = L"  \t      \ttest   whitespace     removal    \r\n";
    wstring output = L"test   whitespace     removal";
    StringUtility::TrimWhitespaces(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeTrimWhitespace2)
{
    wstring input = L"  \t  \n   Just at the beginning";
    wstring output = L"Just at the beginning";
    StringUtility::TrimWhitespaces(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeTrimSpace1)
{
    wstring input = L"        ";
    wstring output = L"";
    StringUtility::TrimSpaces(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeTrimSpace2)
{
    wstring input = L"Just at the end        ";
    wstring output = L"Just at the end";
    StringUtility::TrimSpaces(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeToUpper1)
{
    wstring input = L"This is a string like ANY other.";
    wstring output = L"THIS IS A STRING LIKE ANY OTHER.";
    StringUtility::ToUpper(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeToUpper2)
{
    string input = "This is a string like ANY other.";
    string output = "THIS IS A STRING LIKE ANY OTHER.";
    StringUtility::ToUpper(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeToLower1)
{
    wstring input = L"This is ANOTHER string like ANY other.";
    wstring output = L"this is another string like any other.";
    StringUtility::ToLower(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(SmokeToLower2)
{
    string input = "This is ANOTHER string like ANY other.";
    string output = "this is another string like any other.";
    StringUtility::ToLower(input);
    VERIFY_IS_TRUE(input == output);
}

BOOST_AUTO_TEST_CASE(StartsWithEndsWith)
{
    string s1 = "test";
    VERIFY_IS_TRUE(StringUtility::StartsWith(s1, string("t")));
    VERIFY_IS_TRUE(StringUtility::StartsWith(s1, string("test")));
    VERIFY_IS_TRUE(!StringUtility::StartsWith(s1, string("test1")));

    VERIFY_IS_TRUE(StringUtility::EndsWithCaseInsensitive(s1, string("T")));
    VERIFY_IS_TRUE(StringUtility::EndsWithCaseInsensitive(s1, string("Test")));
    VERIFY_IS_TRUE(!StringUtility::EndsWithCaseInsensitive(s1, string("*Test")));
}

BOOST_AUTO_TEST_CASE(UtfConversion)
{
    const wstring utf16Input = L"this is a test string";
    string utf8Output;
    StringUtility::Utf16ToUtf8(utf16Input, utf8Output);
    Trace.WriteInfo(TraceType, "utf8Output = {0}", utf8Output); 

    const string utf8OutputExpected = "this is a test string";
    BOOST_REQUIRE(utf8Output == utf8OutputExpected);

    wstring utf16Output;
    StringUtility::Utf8ToUtf16(utf8Output, utf16Output);
    Trace.WriteInfo(TraceType, "utf16Output =  {0}", utf16Output); 
    BOOST_REQUIRE(utf16Output == utf16Input);
}

BOOST_AUTO_TEST_CASE(CompareDigitsAsNumbers)
{
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)0, (const wchar_t*)L"ac") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)0, (const wchar_t*)0) == 0);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"a", (const wchar_t*)0) == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab", (const wchar_t*)L"ac") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab", (const wchar_t*)L"a") == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)"1", (const wchar_t*)L"2") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"0001", (const wchar_t*)L"2") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"10", (const wchar_t*)L"2") == 1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"10", (const wchar_t*)L"0002") == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab1", (const wchar_t*)L"ab2") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab0001", (const wchar_t*)L"ab2") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab10", (const wchar_t*)L"ab2") == 1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab0a", (const wchar_t*)L"ab0a") == 0);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab000a", (const wchar_t*)L"ab0a") == 0);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab2a", (const wchar_t*)L"ab11a") == -1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab222a", (const wchar_t*)L"ab222c") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab000222a", (const wchar_t*)L"ab222c") == -1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab000222a000", (const wchar_t*)L"ab222a00") == 0);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"ab000222a10", (const wchar_t*)L"ab222a002") == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"2", (const wchar_t*)L"10") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"10", (const wchar_t*)L"2") == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"UD2", (const wchar_t*)L"UD10") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"UD10", (const wchar_t*)L"UD2") == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"fabric:/UD/2", (const wchar_t*)L"fabric:/UD/10") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"fabric:/UD/10", (const wchar_t*)L"fabric:/UD/2") == 1);

    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"fabric:/UD/UD2", (const wchar_t*)L"fabric:/UD/UD10") == -1);
    VERIFY_IS_TRUE(StringUtility::CompareDigitsAsNumbers((const wchar_t*)L"fabric:/UD/UD10", (const wchar_t*)L"fabric:/UD/UD2") == 1);
}

BOOST_AUTO_TEST_SUITE_END()
