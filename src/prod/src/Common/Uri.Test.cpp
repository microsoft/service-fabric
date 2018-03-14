// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Common/Uri.h"
#include "AssertWF.h"

using namespace Common;

namespace Common
{
    class UriTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(UriTestSuite,UriTest)

    BOOST_AUTO_TEST_CASE(UriCreationTest)
    {
        Uri one(L"scheme");
        Uri two(L"scheme", L"auth");
        Uri three(L"scheme", L"auth", L"path");
        Uri four(L"scheme", L"auth", L"path/morePath");
        Uri five(L"wf", L"foo", L"bar");

        CODING_ERROR_ASSERT(one.Scheme == two.Scheme);
        CODING_ERROR_ASSERT(one.Segments.empty() && two.Segments.empty());
        CODING_ERROR_ASSERT(two.Authority == three.Authority);
        CODING_ERROR_ASSERT(one.Authority != two.Authority);
        CODING_ERROR_ASSERT(one.Path != four.Path);
        CODING_ERROR_ASSERT(three.Segments.size() == 1);
        CODING_ERROR_ASSERT(four.Segments.size() == 2);
        CODING_ERROR_ASSERT(one.Scheme != five.Scheme);
        CODING_ERROR_ASSERT(two.Authority != five.Authority);
        CODING_ERROR_ASSERT(three.Path != five.Path);
    }

    BOOST_AUTO_TEST_CASE(UriSchemeCompareTest)
    {
        Uri one(L"red");
        Uri two(L"Red");
        Uri three(L"blue");
        Uri four(L"blue");

        CODING_ERROR_ASSERT(one == two);
        CODING_ERROR_ASSERT(three == four);
        CODING_ERROR_ASSERT(one != three);
    }

    BOOST_AUTO_TEST_CASE(UriAuthorityCompareTest)
    {
        Uri one(L"green", L"square");
        Uri two(L"green", L"Square");            
        Uri three(L"green", L"square");                     
        Uri four(L"green", L"square", L"light");
        Uri five(L"green", L"Square", L"light");
        Uri six(L"green", L"square", L"dark");
        Uri seven(L"green", L"circle", L"light");
        Uri eight(L"yellow", L"square", L"light");

        CODING_ERROR_ASSERT(one == three);
        CODING_ERROR_ASSERT(one == two);
        CODING_ERROR_ASSERT(one.Scheme == four.Scheme);
        CODING_ERROR_ASSERT(one.Authority == four.Authority);
        CODING_ERROR_ASSERT(one.Scheme == five.Scheme);
        CODING_ERROR_ASSERT(one.Authority == five.Authority);
        CODING_ERROR_ASSERT(five.Scheme == one.Scheme);
        CODING_ERROR_ASSERT(five.Authority == one.Authority);
        CODING_ERROR_ASSERT(five.Scheme == six.Scheme);
        CODING_ERROR_ASSERT(five.Authority == six.Authority);
        CODING_ERROR_ASSERT(six.Scheme == seven.Scheme);
        CODING_ERROR_ASSERT(six.Authority != seven.Authority);
        CODING_ERROR_ASSERT(six.Scheme != eight.Scheme);
        CODING_ERROR_ASSERT(six.Authority == eight.Authority);
    }

    BOOST_AUTO_TEST_CASE(UriPathCompareTest)
    {
        Uri one(L"gold", L"triangle", L"dim");
        Uri two(L"gold", L"triangle", L"dim");
        Uri three(L"gold", L"triangle", L"bright");
        Uri four(L"gold", L"triangle", L"Dim");
        Uri five(L"gold", L"octagon", L"bright");

        CODING_ERROR_ASSERT(one == two);
        CODING_ERROR_ASSERT(one != three);
        CODING_ERROR_ASSERT(one != four);
        CODING_ERROR_ASSERT(one != five);
        CODING_ERROR_ASSERT(three != five);
    }

    BOOST_AUTO_TEST_CASE(UriToStringTest)
    {
        Uri one(L"orange", L"pentagon", L"cloudy");
        Uri two(L"purple", L"hexagon");
        Uri three(L"fuchsia", L"");

        CODING_ERROR_ASSERT(one.ToString() == L"orange://pentagon/cloudy");
        CODING_ERROR_ASSERT(two.ToString() == L"purple://hexagon");
        CODING_ERROR_ASSERT(three.ToString() == L"fuchsia:");
    }

    BOOST_AUTO_TEST_CASE(UriParseTest)
    {
        std::wstring zero = L"amber:";
        std::wstring one = L"pink://";
        std::wstring two = L"brown://decagon";
        std::wstring three = L"black://dodecagon/sparkly";
        std::wstring four = L"olive:/";
        std::wstring five = L"purple//septagon";
        std::wstring six = L"magenta:/torus/drab";
        std::wstring seven = L"oval/pale";
        std::wstring eight = L"black://purple/white/pink";
        std::wstring nine = L"green://yellow:808/rhombus";
        std::wstring ten = L"plaid://127.0.0.1:9000";
        std::wstring eleven = L"chartreuse://[2001:4898:0:fff:0:5efe:10.121.33.93]:103";
        std::wstring twelve = L"red://grey:987654321/";
        std::wstring legal13 = L"indigo://white/space";
        std::wstring legal14 = L"\t\r\n indigo://white/space \n\r\t";
        std::wstring legal15 = L"indigo://white/ space";
        std::wstring illegal16 = L"indigo://wh ite/space";
        std::wstring illegal17 = L"indi go://white/space";
        std::wstring legal18 = L"mauve://circle/no%20white%20space";
        std::wstring illegal19 = L"mauve://circle/no%20white%2/space";
        Uri temp(L"white", L"rhombus", L"brilliant");
        std::wstring tempS = temp.ToString();

        CODING_ERROR_ASSERT(Uri::TryParse(zero, temp));
        CODING_ERROR_ASSERT(temp.ToString() == zero);
        CODING_ERROR_ASSERT(Uri::TryParse(one, temp));
        CODING_ERROR_ASSERT(temp.ToString() == one);
        CODING_ERROR_ASSERT(Uri::TryParse(two, temp));
        CODING_ERROR_ASSERT(temp.ToString() == two);
        CODING_ERROR_ASSERT(Uri::TryParse(three, temp));
        CODING_ERROR_ASSERT(temp.ToString() == three);
        CODING_ERROR_ASSERT(Uri::TryParse(eight, temp));
        CODING_ERROR_ASSERT(temp.ToString() == eight);
        CODING_ERROR_ASSERT(temp.Segments.size() == 2);
        CODING_ERROR_ASSERT(Uri::TryParse(four, temp));
        CODING_ERROR_ASSERT(temp.ToString() == four);
        CODING_ERROR_ASSERT(Uri::TryParse(six, temp));
        CODING_ERROR_ASSERT(temp.ToString() == six);
        CODING_ERROR_ASSERT(Uri::TryParse(legal13, temp));
        CODING_ERROR_ASSERT(temp.ToString() == legal13);
        CODING_ERROR_ASSERT(Uri::TryParse(legal14, temp));
        CODING_ERROR_ASSERT(temp.ToString() == legal13); // mismatch intentional: 13 is same as 14 but without spaces
        CODING_ERROR_ASSERT(Uri::TryParse(legal15, temp));
        CODING_ERROR_ASSERT(temp.ToString() == legal15);
        CODING_ERROR_ASSERT(Uri::TryParse(legal18, temp));
        CODING_ERROR_ASSERT(temp.ToString() == legal18);

        CODING_ERROR_ASSERT(Uri::TryParse(nine, temp));
        CODING_ERROR_ASSERT(temp.ToString() == nine);
        CODING_ERROR_ASSERT(temp.Port == 808);

        CODING_ERROR_ASSERT(Uri::TryParse(ten, temp));
        CODING_ERROR_ASSERT(temp.ToString() == ten);
        CODING_ERROR_ASSERT(temp.Port == 9000);

        CODING_ERROR_ASSERT(Uri::TryParse(eleven, temp));
        CODING_ERROR_ASSERT(temp.ToString() == eleven);
        CODING_ERROR_ASSERT(temp.Port == 103);

        temp = Uri(L"white", L"rhombus", L"brilliant");

        CODING_ERROR_ASSERT(!Uri::TryParse(five, temp));
        CODING_ERROR_ASSERT(temp.ToString() == tempS);
        CODING_ERROR_ASSERT(!Uri::TryParse(seven, temp));
        CODING_ERROR_ASSERT(temp.ToString() == tempS);
        CODING_ERROR_ASSERT(!Uri::TryParse(twelve, temp));
        CODING_ERROR_ASSERT(temp.ToString() == tempS);
        CODING_ERROR_ASSERT(!Uri::TryParse(illegal16, temp));
        CODING_ERROR_ASSERT(temp.ToString() == tempS);
        CODING_ERROR_ASSERT(!Uri::TryParse(illegal17, temp));
        CODING_ERROR_ASSERT(temp.ToString() == tempS);
        CODING_ERROR_ASSERT(!Uri::TryParse(illegal19, temp));
        CODING_ERROR_ASSERT(temp.ToString() == tempS);
    }

    BOOST_AUTO_TEST_CASE(LessThanTest)
    {
        Uri one(L"bronze");
        Uri two(L"Copper");
        Uri three(L"silver");
        Uri four(L"SILVER");

        CODING_ERROR_ASSERT(one < two && two < three);
        CODING_ERROR_ASSERT(!(three < four));

        Uri five(L"bronze", L"parallelogram");
        Uri six(L"bronze", L"Rectangle");
        Uri seven(L"bronze", L"trapezoid");
        Uri eight(L"bronze", L"TRAPEZOID");

        CODING_ERROR_ASSERT(five < six && six < seven);
        CODING_ERROR_ASSERT(!(seven < eight));

        Uri nine(L"bronze", L"parallelogram", L"fiery");
        Uri ten(L"bronze", L"parallelogram", L"iridescent");
        Uri eleven(L"bronze", L"parallelogram", L"shiny");
        Uri twelve(L"bronze", L"parallelogram", L"SHINY");

        CODING_ERROR_ASSERT(nine < ten && ten < eleven);
        CODING_ERROR_ASSERT(twelve < eleven);
    }

    BOOST_AUTO_TEST_CASE(UnicodeTest)
    {
        Uri uri;

        // *** arbitrary unicode characters
        std::wstring unicode(L"unicodetest1:/");
        unicode.push_back(L'\u9EAD');
        unicode.push_back(L'\uAEEF');
        unicode.push_back(L'/');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uCACE');
        unicode.push_back(L'\u3060'); // Hiragana

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);

        // *** combining diacritical marks
        unicode = L"unicodetest2:/";
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u0300');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u032F');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u036F');
        unicode.push_back(L'/');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u0300');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u032F');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u036F');

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);

        // *** combining diacritical marks supplement
        unicode = L"unicodetest3:/";
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u1DC0');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u1DA3');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u1DFF');
        unicode.push_back(L'/');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u1DC0');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u1DA3');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u1DFF');

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);

        // *** combining half marks
        unicode = L"unicodetest4:/";
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uFE20');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uFE2C');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uFE2F');
        unicode.push_back(L'/');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uFE20');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uFE2C');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\uFE2F');

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);

        // *** combining diacritical marks for symbols
        unicode = L"unicodetest5:/";
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u20D0');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u20EB');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u20FF');
        unicode.push_back(L'/');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u20D0');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u20EB');
        unicode.push_back(L'\uB00D');
        unicode.push_back(L'\u20FF');

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);
    }

    BOOST_AUTO_TEST_CASE(UnicodeSurrogatePairsTest)
    {
        Uri uri;

        // *** surrogate pairs in the valid range
        std::wstring unicode(L"surrogatepairs1:/");

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xD800));
        unicode.push_back(static_cast<wchar_t>(0xDC00));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDAAA));
        unicode.push_back(static_cast<wchar_t>(0xDEEB));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDBFF));
        unicode.push_back(static_cast<wchar_t>(0xDFFF));

        unicode.push_back(L'/');

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xD800));
        unicode.push_back(static_cast<wchar_t>(0xDC00));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDAAA));
        unicode.push_back(static_cast<wchar_t>(0xDEEB));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDBFF));
        unicode.push_back(static_cast<wchar_t>(0xDFFF));

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);

        // *** reverse byte order of the above (no byte ordering is assumed)
        unicode = L"surrogatepairs2:/";

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xDC00));
        unicode.push_back(static_cast<wchar_t>(0xD800));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDEEB));
        unicode.push_back(static_cast<wchar_t>(0xDAAA));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDFFF));
        unicode.push_back(static_cast<wchar_t>(0xDBFF));

        unicode.push_back(L'/');

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xDC00));
        unicode.push_back(static_cast<wchar_t>(0xD800));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDEEB));
        unicode.push_back(static_cast<wchar_t>(0xDAAA));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDFFF));
        unicode.push_back(static_cast<wchar_t>(0xDBFF));

        VERIFY_IS_TRUE(Uri::TryParse(unicode, uri));
        VERIFY_IS_TRUE(uri.ToString() == unicode);

        // *** missing leading code
        unicode = L"surrogatepairs3:/";

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xDC00));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDEEB));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDFFF));

        unicode.push_back(L'/');

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xDC00));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDEEB));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDFFF));

        VERIFY_IS_FALSE(Uri::TryParse(unicode, uri));

        // *** missing trailing code
        unicode = L"surrogatepairs4:/";

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xD800));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDAAA));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDBFF));

        unicode.push_back(L'/');

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xD800));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDAAA));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDBFF));

        VERIFY_IS_FALSE(Uri::TryParse(unicode, uri));

        // *** code is surrounded by non-surrogate code
        unicode = L"surrogatepairs5:/";

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xB00D));
        unicode.push_back(static_cast<wchar_t>(0xDC00));
        unicode.push_back(static_cast<wchar_t>(0xB00D));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDEEB));
        unicode.push_back(static_cast<wchar_t>(0xB00D));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDFFF));
        unicode.push_back(static_cast<wchar_t>(0xB00D));

        unicode.push_back(L'/');

        // first in range
        unicode.push_back(static_cast<wchar_t>(0xB00D));
        unicode.push_back(static_cast<wchar_t>(0xD800));
        unicode.push_back(static_cast<wchar_t>(0xB00D));

        // arbitrary in range
        unicode.push_back(static_cast<wchar_t>(0xDAAA));
        unicode.push_back(static_cast<wchar_t>(0xB00D));

        // last in range
        unicode.push_back(static_cast<wchar_t>(0xDBFF));
        unicode.push_back(static_cast<wchar_t>(0xB00D));

        VERIFY_IS_FALSE(Uri::TryParse(unicode, uri));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
