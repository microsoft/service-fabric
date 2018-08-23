// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include "IDNConversionUtility.h"
#endif
const ULONG MAX_DNS_LABEL_SIZE = 63;
namespace DNS { namespace Test
{
    class DnsParserTests
    {
    public:
        DnsParserTests() { BOOST_REQUIRE(Setup()); }
        ~DnsParserTests() { BOOST_REQUIRE(Cleanup()); }

        TEST_CLASS_SETUP(Setup);
        TEST_CLASS_CLEANUP(Cleanup);

    protected:
        KAllocator& GetAllocator() { return _pRuntime->NonPagedAllocator(); }

    protected:
        Runtime* _pRuntime;
        IDnsParser::SPtr _spParser;
    };

    bool DnsParserTests::Setup()
    {
        _pRuntime = Runtime::Create();
        if (_pRuntime == nullptr)
        {
            return false;
        }

        DNS::CreateDnsParser(/*out*/_spParser, GetAllocator());
        return true;
    }

    bool DnsParserTests::Cleanup()
    {
        _spParser = nullptr;

        _pRuntime->Delete();

        return true;
    }

    void DeserializeQuestionHelper(
        __in IDnsParser& parser,
        __in LPCWSTR szQuestion,
        __in KBuffer& buffer,
        __in bool expectSuccess,
        __in DnsTextEncodingType encodingType
    )
    {
        ULONG size;
        DnsHelper::SerializeQuestion(parser, szQuestion, buffer, DNS_TYPE_A, /*out*/size, encodingType);

        IDnsMessage::SPtr spOut;
        bool result = parser.Deserialize(spOut, buffer, size);
        if (result != expectSuccess)
        {
            VERIFY_FAIL_FMT(L"Deserialize test for question <%s> returned <%d>, expected result <%d>",
                szQuestion, result, expectSuccess);
        }
    }
    void charTowChar (LPSTR output, int length, LPWSTR widepointer)
    {
        for (int i = 0; i < length; i++) 
        {
            widepointer[i] = (wchar_t) output[i];
        }
        return;
    }

    bool testIDNtoUnicode (LPWSTR input, int inputSize, LPWSTR output, int outputSize, int expectedSize, LPWSTR expectedResult)
    {
        int charReceived = IdnToUnicode (0, input, inputSize, output, outputSize);
        if (charReceived != expectedSize) 
        {
            return false;
        }
        if (outputSize == 0 || output == NULL) 
        {
            return true;
        }
        if (memcmp (expectedResult, output, sizeof (wchar_t) *charReceived) == 0) 
        {
            return true;
        } 
        else 
        {
            return false;
        }
    }
    
    bool testIDNtoAscii (LPWSTR input, int inputSize, LPWSTR outputString, int outputSize, int expectedSize, LPWSTR expectedResult)
    {
        int charsReceived = IdnToAscii (0, input, inputSize, outputString, outputSize);

        if (charsReceived != expectedSize) 
        {
            return false;
        }
        if (outputSize == 0 || outputString ==NULL) 
        {
            return true;
        }
        if (memcmp (expectedResult, outputString, sizeof (wchar_t) *charsReceived) == 0) 
        {
            return true;
        } 
        else 
        {
            return false;
        }
    }
    BOOST_FIXTURE_TEST_SUITE(DnsParserTestSuite, DnsParserTests);

    BOOST_AUTO_TEST_CASE(TestInvalidDnsName)
    {
        KBuffer::SPtr spBuffer;
        KBuffer::Create(4096, /*out*/spBuffer, GetAllocator());

        DeserializeQuestionHelper(*_spParser, L".", *spBuffer, false, DnsTextEncodingTypeUTF8);

        //These are not RFC compliant but we let them through
        DeserializeQuestionHelper(*_spParser, L"/?", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar{", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar|", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar}", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar~", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar[", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar]", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar\\", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar^", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar:", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar;", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar<", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar>", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar=", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar?", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar&", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar@", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar!", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar#", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar%", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar^", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar(", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar)", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar+", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar/", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar,", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar*", *spBuffer, true, DnsTextEncodingTypeUTF8);
        DeserializeQuestionHelper(*_spParser, L"invalidchar_", *spBuffer, true, DnsTextEncodingTypeUTF8);

        // INVALID-NAME
        {
            LPCWSTR arrInvalidName[] = {
                L".startswithdot",
                L"two..dots",
                L"..two..dots",
                L"label.longerthan63chars.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
                L"name.longerthan255chars.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxxxxxxx",
            };
            for (ULONG i = 0; i < ARRAYSIZE(arrInvalidName); i++)
            {
                if (DNS::IsDnsNameValid(arrInvalidName[i]) != DNSNAME_INVALID_NAME)
                {
                    VERIFY_FAIL(arrInvalidName[i]);
                }
            }
        }

        // INVALID-CHAR
        {
            LPCWSTR arrInvalidChar[] = {
                L"invalidchar{", L"invalidchar|", L"invalidchar}", L"invalidchar~", L"invalidchar[", L"invalidchar]",
                L"invalidchar\\", L"invalidchar^", L"invalidchar:", L"invalidchar;", L"invalidchar<", L"invalidchar>",
                L"invalidchar=", L"invalidchar?", L"invalidchar&", L"invalidchar@", L"invalidchar!", L"invalidchar#",
                L"invalidchar%", L"invalidchar^", L"invalidchar(", L"invalidchar)", L"invalidchar+", L"invalidchar/",
                L"invalidchar,", L"invalidchar*", L"invalidchar "
            };
            for (ULONG i = 0; i < ARRAYSIZE(arrInvalidChar); i++)
            {
                if (DNS::IsDnsNameValid(arrInvalidChar[i]) != DNSNAME_INVALID_NAME_CHAR)
                {
                    VERIFY_FAIL(arrInvalidChar[i]);
                }
            }
        }

        // NON-RFC
        {
            LPCWSTR arrNonRFC[] = {
                L"NonRFC_Name"
            };
            for (ULONG i = 0; i < ARRAYSIZE(arrNonRFC); i++)
            {
                if (DNS::IsDnsNameValid(arrNonRFC[i]) != DNSNAME_NON_RFC_NAME)
                {
                    VERIFY_FAIL(arrNonRFC[i]);
                }
            }
        }
    }

    BOOST_AUTO_TEST_CASE(TestValidDnsName)
    {
        KBuffer::SPtr spBuffer;
        KBuffer::Create(4096, /*out*/spBuffer, GetAllocator());

        LPCWSTR arrValid[] = {
            L".", L"foo", L"foo.bar", L"foo.bar.xxx", L"foo.bar.xxx.yyy", L"foo-bar",
            L"12345", L"label.63chars.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            L"name.253chars.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            L"UNICODE.\u043f\u0412.\u0431\u0432"
        };
        for (ULONG i = 0; i < ARRAYSIZE(arrValid); i++)
        {
            if (DNS::IsDnsNameValid(arrValid[i]) != DNSNAME_VALID)
            {
                VERIFY_FAIL(arrValid[i]);
            }
        }

        // Skip "."
        for (ULONG i = 1; i < ARRAYSIZE(arrValid); i++)
        {
            DeserializeQuestionHelper(*_spParser, arrValid[i], *spBuffer, true, DnsTextEncodingTypeUTF8);
        }
    }
    
    BOOST_AUTO_TEST_CASE(TestValidDnsNameWithIDN)
    {
        KBuffer::SPtr spBuffer;
        KBuffer::Create(4096, /*out*/spBuffer, GetAllocator());

        LPCWSTR arrValid[] = {
            L".", L"foo", L"foo.bar", L"foo.bar.xxx", L"foo.bar.xxx.yyy", L"foo-bar",
            L"12345", L"label.63chars.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            L"name.253chars.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxx.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            L"UNICODE.\u043f\u0412.\u0431\u0432"
        };
        for (ULONG i = 0; i < ARRAYSIZE(arrValid); i++)
        {
            if (DNS::IsDnsNameValid(arrValid[i]) != DNSNAME_VALID)
            {
                VERIFY_FAIL(arrValid[i]);
            }
        }

        // Skip "."
        for (ULONG i = 1; i < ARRAYSIZE(arrValid); i++)
        {
            DeserializeQuestionHelper(*_spParser, arrValid[i], *spBuffer, true, DnsTextEncodingTypeIDN);
        }
    }
    
    BOOST_AUTO_TEST_CASE (TestIdntoUnicodeConversion)
    {
        const int bufferSize = 20;
        LPWSTR output = new wchar_t[bufferSize];

        //valid input with input size
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", 11, output, bufferSize, 2, L"\u9ede\u770b") == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", 24, output, bufferSize, 11, L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679") == true);
        //valid input including null
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", 12, output, bufferSize, 3, L"\u9ede\u770b") == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", 25, output, bufferSize, 12, L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679") == true);
        //valid input with null termination
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", -1, output, bufferSize, 3, L"\u9ede\u770b") == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", -1, output, bufferSize, 12, L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679") == true);
        //valid input with outputsize = 0
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", 11, output, 0, 2, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", 24, output, 0, 11, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", -1, output, 0, 3, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", -1, output, 0, 12, NULL) == true);
        //size = 0 output buffer is null
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", 11, NULL, 0, 2, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", 24, NULL, 0, 11, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", -1, NULL, 0, 3, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", -1, NULL, 0, 12, NULL) == true);
        //valid ascii characters
        BOOST_CHECK (testIDNtoUnicode (L"microsoft", 9, output, bufferSize, 9, L"microsoft") == true);
        BOOST_CHECK (testIDNtoUnicode (L"microsoft", 10, output, bufferSize, 10, L"microsoft") == true);
        BOOST_CHECK (testIDNtoUnicode (L"microsoft", -1, output, bufferSize, 10, L"microsoft") == true);
        //Invalid Inputs
        //Utf-8 encoded string
        BOOST_CHECK (testIDNtoUnicode (L"\xE9\xBB\x9E\xE7\x9C\x8B", 6, output, bufferSize, 0, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1de2w3e", 12, output, bufferSize, 0, NULL) == true);
        BOOST_CHECK(testIDNtoAscii(L"microsoft\xE9\xBB\x9E\xE7\x9C\x8B", 15, output, bufferSize, 0, NULL) == true);
        //Invalid output buffer
        LPWSTR smallBuffer = new wchar_t[10];
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", 11, NULL, bufferSize, 0, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", 24, smallBuffer, 10, 0, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--c1yn36f", -1, NULL, bufferSize, 0, NULL) == true);
        BOOST_CHECK (testIDNtoUnicode (L"xn--lgbb5ai4etayb65bjpmm", -1, smallBuffer, 10, 0, NULL) == true);
    }
    
    BOOST_AUTO_TEST_CASE (TestIdntoAsciiConversion)
    {
        const int bufferSize = 30;
        LPWSTR output = new wchar_t[bufferSize];
        //UNicode to ASCII test
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", 2, output, bufferSize, 11, L"xn--c1yn36f") == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", 11, output, bufferSize, 24, L"xn--lgbb5ai4etayb65bjpmm") == true);
        //valid input including null
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", 3, output, bufferSize, 12, L"xn--c1yn36f") == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", 12, output, bufferSize, 25, L"xn--lgbb5ai4etayb65bjpmm") == true);
        //valid input with null termination
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", -1, output, bufferSize, 12, L"xn--c1yn36f") == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", -1, output, bufferSize, 25, L"xn--lgbb5ai4etayb65bjpmm") == true);
        //valid input with outputsize = 0
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", 2, output, 0, 11, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", 11, output, 0, 24, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", -1, output, 0, 12, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", -1, output, 0, 25, NULL) == true);
        //size = 0 output buffer is null
        BOOST_CHECK(testIDNtoUnicode(L"xn--c1yn36f", 11, NULL, 0, 2, NULL) == true);
        BOOST_CHECK(testIDNtoUnicode(L"xn--lgbb5ai4etayb65bjpmm", 24, NULL, 0, 11, NULL) == true);
        BOOST_CHECK(testIDNtoUnicode(L"xn--c1yn36f", -1, NULL, 0, 3, NULL) == true);
        BOOST_CHECK(testIDNtoUnicode(L"xn--lgbb5ai4etayb65bjpmm", -1, NULL, 0, 12, NULL) == true);
        //valid ascii characters
        BOOST_CHECK (testIDNtoAscii (L"microsoft", 9, output, bufferSize, 9, L"microsoft") == true);
        BOOST_CHECK (testIDNtoAscii (L"microsoft", 10, output, bufferSize, 10, L"microsoft") == true);
        BOOST_CHECK (testIDNtoAscii (L"microsoft", -1, output, bufferSize, 10, L"microsoft") == true);
        //Invalid Input
        //Utf-8 encoded string
        BOOST_CHECK (testIDNtoAscii (L"\xE9\xBB\x9E\xE7\x9C\x8B", 6, output, bufferSize, 0, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"microsoft\xE9\xBB\x9E\xE7\x9C\x8B", 15, output, bufferSize, 0, NULL) == true);
        //Invalid output buffer
        LPWSTR smallBuffer = new wchar_t[10];
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", 2, NULL, bufferSize, 0, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", 11, smallBuffer, 10, 0, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"\u9ede\u770b", -1, NULL, bufferSize, 0, NULL) == true);
        BOOST_CHECK (testIDNtoAscii (L"\u0645\u0627\u0626\u06CC\u06A9\u0631\u0648\u0633\u0648\u0641\u0679", -1, smallBuffer, 10, 0, NULL) == true);
    }

    BOOST_AUTO_TEST_SUITE_END()
}}
