// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// CRC values are verified with http://www.sunshine2k.de/coding/javascript/crc/crc_js.html

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(crctest)


    BOOST_AUTO_TEST_CASE(basic32)
    {
        ENTER;

        string s1("this is a test string");
        crc32 c1;
        c1.AddData(s1.c_str(), s1.length());
        Trace.WriteInfo(TraceType, "c1 = {0:x}", c1.Value());
        VERIFY_IS_TRUE(c1.Value() == 0xFC86D6B5);

        crc32 c1_2(s1.c_str(), s1.length());
        Trace.WriteInfo(TraceType, "c1_2 = {0:x}", c1_2.Value()); 
        VERIFY_IS_TRUE(c1.Value() == c1_2.Value());

        string s2("this is a test strinf"); // s1 and s2 differs by a single bit
        crc32 c2(s2.c_str(), s2.length());
        Trace.WriteInfo(TraceType, "c2 = {0:x}", c2.Value()); 
        VERIFY_IS_TRUE(c1.Value() != c2.Value());
        VERIFY_IS_TRUE(c2.Value() == 0x8B81E623);

        string s3("Garbled data@34@!@%@#"); // s3 is very different from s1 and s2
        crc32 c3;
        c3.AddData(s3.c_str(), s3.length());
        Trace.WriteInfo(TraceType, "c3 = {0:x}", c3.Value()); 
        VERIFY_IS_TRUE(c1.Value() != c3.Value());
        VERIFY_IS_TRUE(c2.Value() != c3.Value());
        VERIFY_IS_TRUE(c3.Value() == 0x582D102A);

        string s1_part2("this is a part 2 of s1");
        c1.AddData(s1_part2.c_str(), s1_part2.length());
        Trace.WriteInfo(TraceType, "c1 = {0:x}", c1.Value()); 
        VERIFY_IS_TRUE(c1.Value() != c1_2.Value());
        VERIFY_IS_TRUE(c1.Value() == 0x36AF107A);

        c1_2.AddData(s1_part2.c_str(), s1_part2.length());
        Trace.WriteInfo(TraceType, "c1_2 = {0:x}", c1_2.Value()); 
        VERIFY_IS_TRUE(c1.Value() == c1_2.Value());

        string s4 = s1 + s1_part2;
        crc32 c4;
        c4.AddData(s4.c_str(), s4.length());
        Trace.WriteInfo(TraceType, "c4 = {0:x}", c4.Value()); 
        VERIFY_IS_TRUE(c1.Value() == c4.Value());

        string s5 = s2 + s1_part2; // s5 and s4 differs by a single bit
        crc32 c5;
        c5.AddData(s5.c_str(), s5.length());
        Trace.WriteInfo(TraceType, "c5 = {0:x}", c5.Value()); 
        VERIFY_IS_TRUE(c1.Value() != c5.Value());
        VERIFY_IS_TRUE(c5.Value() == 0xADDCFAAE);

        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(basic8)
    {
        ENTER;

        string s1("this is a test string");
        crc8 c1;
        c1.AddData(s1.c_str(), s1.length());
        Trace.WriteInfo(TraceType, "c1 = {0:x}", c1.Value());
        VERIFY_IS_TRUE(c1.Value() == 0x17);

        crc8 c1_2(s1.c_str(), s1.length());
        Trace.WriteInfo(TraceType, "c1_2 = {0:x}", c1_2.Value());
        VERIFY_IS_TRUE(c1.Value() == c1_2.Value());

        string s2("this is a test strinf"); // s1 and s2 differs by a single bit
        crc8 c2(s2.c_str(), s2.length());
        Trace.WriteInfo(TraceType, "c2 = {0:x}", c2.Value());
        VERIFY_IS_TRUE(c1.Value() != c2.Value());
        VERIFY_IS_TRUE(c2.Value() == 0x10);

        string s3("Garbled data@34@!@%@#"); // s3 is very different from s1 and s2
        crc8 c3;
        c3.AddData(s3.c_str(), s3.length());
        Trace.WriteInfo(TraceType, "c3 = {0:x}", c3.Value());
        VERIFY_IS_TRUE(c1.Value() != c3.Value());
        VERIFY_IS_TRUE(c2.Value() != c3.Value());
        VERIFY_IS_TRUE(c3.Value() == 0x9f);

        string s1_part2("this is a part 2 of s1");
        c1.AddData(s1_part2.c_str(), s1_part2.length());
        Trace.WriteInfo(TraceType, "c1 = {0:x}", c1.Value());
        VERIFY_IS_TRUE(c1.Value() != c1_2.Value());
        VERIFY_IS_TRUE(c1.Value() == 0x7C);

        c1_2.AddData(s1_part2.c_str(), s1_part2.length());
        Trace.WriteInfo(TraceType, "c1_2 = {0:x}", c1_2.Value());
        VERIFY_IS_TRUE(c1.Value() == c1_2.Value());

        string s4 = s1 + s1_part2;
        crc8 c4;
        c4.AddData(s4.c_str(), s4.length());
        Trace.WriteInfo(TraceType, "c4 = {0:x}", c4.Value());
        VERIFY_IS_TRUE(c1.Value() == c4.Value());

        string s5 = s2 + s1_part2; // s5 and s4 differs by a single bit
        crc8 c5;
        c5.AddData(s5.c_str(), s5.length());
        Trace.WriteInfo(TraceType, "c5 = {0:x}", c5.Value());
        VERIFY_IS_TRUE(c1.Value() != c5.Value());
        VERIFY_IS_TRUE(c5.Value() == 0xC5);

        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
}
