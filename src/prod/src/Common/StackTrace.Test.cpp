// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/StackTrace.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(StackTraceTests)

    BOOST_AUTO_TEST_CASE(DirectStackTraceTest)
    {
        StackTrace stackTrace;
        stackTrace.CaptureCurrentPosition();
        wstring result = stackTrace.ToString();
        Trace.WriteInfo(TraceType, "{0}", result);
        for(int i = 0; i < result.length(); i++)
        {
            cout << (char) result[i];
        }
        cout << "\n";

        BOOST_REQUIRE(result.find(L"DirectStackTraceTest") != wstring::npos);
    }

#if defined(PLATFORM_UNIX) && DBG
/*
    BOOST_AUTO_TEST_CASE(BackTraceTest)
    {
        BackTrace bt(3);
        auto lineNo = __LINE__ - 1;
        cout << "line number after BackTrace: " << lineNo << endl;

        auto err = bt.ResolveSymbols();
        VERIFY_ARE_EQUAL2(err.ReadValue(), ErrorCodeValue::Success);

        auto exeName = Path::GetFileName(Environment::GetExecutableFileName());
        string exeNameA;
        StringUtility::Utf16ToUtf8(exeName, exeNameA);
        cout << exeNameA << endl;
        Trace.WriteInfo(TraceType, "exeName = {0}", exeNameA);

        for (int i = 0; i < 3; ++i)
        {
            cout << bt.ModuleList()[i] << " : " << bt.FuncAddrList()[i] << ' ' << bt.SrcFileLineNoList()[i] << endl;
            Trace.WriteInfo(TraceType, "{0} : {1} [{2}]", bt.ModuleList()[i], bt.FuncAddrList()[i], bt.SrcFileLineNoList()[i]);
        }

        VERIFY_IS_TRUE(StringUtility::Contains(bt.ModuleList()[0], exeNameA));
        VERIFY_IS_TRUE(StringUtility::Contains(bt.FuncAddrList()[0], string("BackTraceTest")));

        auto expectedFileLineNo = formatString("StackTrace.Test.cpp:{0}", lineNo);
        cout << "expected file and line number: " << expectedFileLineNo << endl;
        cout << "bt.SrcFileLineNoList()[0]: " << bt.SrcFileLineNoList()[0] << endl;
        VERIFY_IS_TRUE(StringUtility::Contains(bt.SrcFileLineNoList()[0], expectedFileLineNo));
    }
*/
    static CallStackRanking callerRanking;

    void __attribute__ ((noinline)) TestCallee()
    {
        callerRanking.Track();
    }

    void __attribute__ ((noinline)) Caller0()
    {
        TestCallee();
    }

    void __attribute__ ((noinline)) Caller1()
    {
        TestCallee();
    }

    void __attribute__ ((noinline)) Caller2()
    {
        TestCallee();
    }

    void __attribute__ ((noinline)) Caller3()
    {
        TestCallee();
    }

    BOOST_AUTO_TEST_CASE(CallerRanking)
    {
        Caller1();

        for(int i = 0; i < 2; ++i)
        {
            Caller2();
        }

        for(int i = 0; i < 3; ++i)
        {
            Caller3();
        }

        callerRanking.ResolveSymbols();

        Trace.WriteInfo(TraceType, "{0}", callerRanking);
        wstring ranking = wformatString("{0}", callerRanking);
        string rankingA;
        StringUtility::Utf16ToUtf8(ranking, rankingA);
        cout << rankingA << endl;

        auto results = callerRanking.Callstacks();
        int count = 1;
        for(auto const & r : results)
        {
            auto callstack = formatString("{0}", r.first);
            cout << "callstack: " << callstack << endl;
            auto expectedFunc = formatString("Caller{0}", r.second);
            cout << "expectedFunc: " << expectedFunc << endl;
            VERIFY_IS_TRUE(StringUtility::Contains(callstack, expectedFunc));
            ++count;
        }
    }

#endif

    BOOST_AUTO_TEST_SUITE_END()
}
