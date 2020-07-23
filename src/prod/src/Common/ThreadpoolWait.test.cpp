// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

Common::StringLiteral const TraceType("ThreadpoolWaitTest");

namespace Common
{
    //
    // Basic test for a thread pool based async wait
    //
    BOOST_AUTO_TEST_SUITE2(ThreadpoolWaitTest)

    BOOST_AUTO_TEST_CASE(BasicTest)
    {
        ManualResetEvent resultWaitHandle(false);
        ManualResetEvent testWaitHandle(false);

        HANDLE testHandle;
        BOOL success = 
                ::DuplicateHandle(
                    ::GetCurrentProcess(),
                    testWaitHandle.GetHandle(),
                    ::GetCurrentProcess(),
                    &testHandle,
                    0,
                    0,
                    DUPLICATE_SAME_ACCESS);

        Trace.WriteInfo(TraceType, "DuplicateHandle returned {0}", success);
        VERIFY_IS_TRUE(success != 0);
        
        Handle handle(testHandle);
        auto tpWait = ThreadpoolWait::Create(
            std::move(handle),
            [&resultWaitHandle](Handle const &, ErrorCode const & errorCode) -> void 
            { 
                Trace.WriteInfo(TraceType, "ThreadpoolWait Callback called");
                resultWaitHandle.Set(); 
                VERIFY_IS_TRUE(errorCode.IsSuccess());
            });
        
        testWaitHandle.Set();
        success = resultWaitHandle.WaitOne(5000);

        VERIFY_IS_TRUE(success != 0);
    }

    BOOST_AUTO_TEST_CASE(TimeoutTest)
    {
        ManualResetEvent resultWaitHandle(false);
        ManualResetEvent testWaitHandle(false);

        HANDLE testHandle;
        BOOL success = ::DuplicateHandle(
            ::GetCurrentProcess(),
            testWaitHandle.GetHandle(),
            ::GetCurrentProcess(),
            &testHandle,
            0,
            0,
            DUPLICATE_SAME_ACCESS);
                   
        Trace.WriteInfo(TraceType, "DuplicateHandle returned {0}", success);
        VERIFY_IS_TRUE(success != 0);
        
        Handle handle(testHandle);
        auto tpWait = ThreadpoolWait::Create(
            std::move(handle),
            [&resultWaitHandle](Handle const &, ErrorCode const & errorCode) -> void 
            { 
                Trace.WriteInfo(TraceType, "ThreadpoolWait Callback called");
                resultWaitHandle.Set();
                VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::Timeout));
            },
            TimeSpan::FromSeconds(3));
        
        success = resultWaitHandle.WaitOne(10000);

        VERIFY_IS_TRUE(success != 0);
    }

    BOOST_AUTO_TEST_CASE(TestDestructionCallbackRace)
    {
        ManualResetEvent testWaitHandle(true);

        for (int i = 0; i < 1000; ++ i)
        {
            HANDLE testHandle;
            BOOL success = 
                    ::DuplicateHandle(
                        ::GetCurrentProcess(),
                        testWaitHandle.GetHandle(),
                        ::GetCurrentProcess(),
                        &testHandle,
                        0,
                        0,
                        DUPLICATE_SAME_ACCESS);
            VERIFY_IS_TRUE(success != 0);

            auto tpWait = ThreadpoolWait::Create(
                Handle(testHandle),
                [](Handle const &, ErrorCode const & errorCode) -> void 
                { 
                    Trace.WriteInfo(TraceType, "ThreadpoolWait Callback called: {0}", errorCode);
                    VERIFY_IS_TRUE(errorCode.IsSuccess());
                });
            tpWait.reset();
        }

        // verification is done in wait callbacks, thus we need to make sure callbacks are
        // either cancelled or completed before allowing this test case to be destructed
        ::Sleep((WORD)TimeSpan::FromSeconds(6).TotalMilliseconds());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
