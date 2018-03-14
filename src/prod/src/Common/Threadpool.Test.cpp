// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

Common::StringLiteral const TraceType("ThreadpoolTest");

namespace Common
{
    //
    // Basic Threadpool test
    //
    BOOST_AUTO_TEST_SUITE(ThreadpoolTest)

//     // Intentional infinite recursion
//#pragma warning ( disable : 717)
//    static void CauseStackOverflow()
//    {
//        char arr[4096];
//        CauseStackOverflow();
//    }
//
//#pragma warning ( default : 717)
//
//    static LONG WINAPI VectorExceptionHandler(PEXCEPTION_POINTERS pException)
//    {
//        if (pException->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
//        {
//            ::RaiseFailFastException(NULL, NULL, 1);
//        }
//
//        return EXCEPTION_CONTINUE_SEARCH;
//    }
//
//    //
//    // This would assert because we are causing a stack overflow. Leaving this
//    // code commented, incase we want to do a quick test later.
//    //
//    BOOST_AUTO_TEST_CASE(ThreadpoolStackOverflowTest)
//    {
//        ManualResetEvent waitHandle(false);
//
//        AddVectoredExceptionHandler(1, VectorExceptionHandler);
//        Threadpool::Post(
//            [&waitHandle]() ->void
//        {
//            CauseStackOverflow();
//        });
//
//        waitHandle.WaitOne(TimeSpan::MaxValue);
//    }
    
    BOOST_AUTO_TEST_CASE(BasicThreadpoolTest)
    {
        ManualResetEvent waitHandle(false);

        Threadpool::Post(
            [&waitHandle]() -> void
        {
            Trace.WriteInfo(TraceType, "Callback()");
            waitHandle.Set();
        });

        waitHandle.WaitOne(TimeSpan::MaxValue);
    }

    //
    // Basic Threadpool test with state
    //
    BOOST_AUTO_TEST_CASE(BasicThreadpoolTestWithState)
    {
        ManualResetEvent waitHandle(false);

        {
            int state = 42;
            Threadpool::Post(
                [&waitHandle, state]() -> void
            {
                Sleep(2000);
                Trace.WriteInfo(TraceType, "Callback({0})", state);
                waitHandle.Set();
            });
        }

        Trace.WriteInfo(TraceType, "Waiting for callback");
        waitHandle.WaitOne(TimeSpan::MaxValue);
    }

    //
    // Delayed callback test
    //
    BOOST_AUTO_TEST_CASE(DelayedCallbackTest)
    {
        ManualResetEvent waitHandle(false);

        Stopwatch stopwatch;

        stopwatch.Start();

        Threadpool::Post(
            [&waitHandle, &stopwatch]() -> void
            {
                stopwatch.Stop();
                Trace.WriteInfo(TraceType, "Callback()");
                waitHandle.Set();
            },
            TimeSpan::FromSeconds(3));

        Trace.WriteInfo(TraceType, "Waiting for callback");
        waitHandle.WaitOne(TimeSpan::MaxValue);

        Trace.WriteInfo(TraceType, "Delay = {0}", stopwatch.Elapsed);
        TimeSpan accuracyMargin = TimeSpan::FromMilliseconds(1000);
        VERIFY_IS_TRUE(TimeSpan::FromSeconds(3) <= (stopwatch.Elapsed + accuracyMargin));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
