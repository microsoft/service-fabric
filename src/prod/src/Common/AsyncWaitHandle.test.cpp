// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    BOOST_AUTO_TEST_SUITE2(AsyncWaitHandleTest)

    BOOST_AUTO_TEST_CASE(TestManualResetEvent)
    {
        AsyncManualResetEvent manualResetEvent;

        AutoResetEvent resultEvent(false);
        manualResetEvent.BeginWaitOne(
            TimeSpan::MaxValue,
            [&manualResetEvent, &resultEvent] (AsyncOperationSPtr const & op)
            {
                ErrorCode result = manualResetEvent.EndWaitOne(op);
                VERIFY_IS_TRUE(result.IsSuccess());
                resultEvent.Set();
            },
            AsyncOperationSPtr());

        manualResetEvent.Set();

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));

        VERIFY_IS_TRUE(manualResetEvent.IsSet());

        VERIFY_IS_TRUE(manualResetEvent.Reset().IsSuccess());

        VERIFY_IS_FALSE(manualResetEvent.IsSet());
    }

    BOOST_AUTO_TEST_CASE(TestAutoResetEvent)
    {
        auto autoResetEvent = std::make_shared<AsyncAutoResetEvent>();

        AutoResetEvent resultEvent(false);
        autoResetEvent->BeginWaitOne(
            TimeSpan::MaxValue,
            [autoResetEvent, &resultEvent] (AsyncOperationSPtr const & op)
            {
                ErrorCode result = autoResetEvent->EndWaitOne(op);
                VERIFY_IS_TRUE(result.IsSuccess());
                resultEvent.Set();
            },
            AsyncOperationSPtr());

        autoResetEvent->Set();

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));

        VERIFY_IS_FALSE(autoResetEvent->IsSet());
    }

    BOOST_AUTO_TEST_CASE(AsyncWaitHandleSyncTest)
    {
        AsyncWaitHandle<true> waitHandle;

        VERIFY_IS_TRUE(waitHandle.Set().IsSuccess());

        VERIFY_IS_TRUE(waitHandle.IsSet());

        VERIFY_IS_TRUE(waitHandle.WaitOne());

        VERIFY_IS_TRUE(waitHandle.Reset().IsSuccess());

        VERIFY_IS_FALSE(waitHandle.IsSet());

        ErrorCode waitResult = waitHandle.Wait(TimeSpan::FromSeconds(1));
        VERIFY_IS_TRUE(waitResult.ReadValue() == ErrorCodeValue::Timeout);
    }

    BOOST_AUTO_TEST_CASE(AsyncWaitHandleBasicAsyncTest)
    {
        AsyncWaitHandle<true> asyncWaitHandle;

        AutoResetEvent resultEvent(false);
        asyncWaitHandle.BeginWaitOne(
            TimeSpan::MaxValue,
            [&asyncWaitHandle, &resultEvent] (AsyncOperationSPtr const & op)
            {
                ErrorCode result = asyncWaitHandle.EndWaitOne(op);
                VERIFY_IS_TRUE(result.IsSuccess());
                resultEvent.Set();
            },
            AsyncOperationSPtr());

        asyncWaitHandle.Set();

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));
    }

    BOOST_AUTO_TEST_CASE(AsyncWaitHandleAsyncTimeoutTest)
    {
        AsyncWaitHandle<true> asyncWaitHandle;

        AutoResetEvent resultEvent(false);
        asyncWaitHandle.BeginWaitOne(
            TimeSpan::FromSeconds(1),
            [&asyncWaitHandle, &resultEvent] (AsyncOperationSPtr const & op)
            {
                ErrorCode result = asyncWaitHandle.EndWaitOne(op);
                VERIFY_IS_TRUE(result.ReadValue() == ErrorCodeValue::Timeout);
                resultEvent.Set();
            },
            AsyncOperationSPtr());

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));
    }

    template <bool ManualReset> void AsyncWaitHandleDestructInCallback()
    {
        auto wh = make_shared<AsyncWaitHandle<ManualReset>>();

        AutoResetEvent callbackCanStart(false);
        AutoResetEvent resultEvent(false);
        wh->BeginWaitOne(
            TimeSpan::MaxValue,
            [wh, &callbackCanStart, &resultEvent] (AsyncOperationSPtr const & op) mutable
            {
                Trace.WriteInfo(TraceType, "callback waiting for action");
                callbackCanStart.WaitOne(); 
                Trace.WriteInfo(TraceType, "callback starting action");
                ErrorCode result = wh->EndWaitOne(op);
                VERIFY_IS_TRUE(result.IsSuccess());
                wh.reset();
                resultEvent.Set();
                Trace.WriteInfo(TraceType, "leaving callback");
            },
            AsyncOperationSPtr());

        //make wh destruct in callback
        wh->Set();
        wh.reset();
        callbackCanStart.Set();

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));
 
    }

    BOOST_AUTO_TEST_CASE(ManualResetAsyncWaitHandleDestructInCallback)
    {
        ENTER;
        AsyncWaitHandleDestructInCallback<true>();
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AutoResetAsyncWaitHandleDestructInCallback)
    {
        ENTER;
        AsyncWaitHandleDestructInCallback<false>();
        LEAVE;
    }

    template <bool ManualReset> void AsyncWaitHandleDestructRaceCallback()
    {
        auto wh = make_shared<AsyncWaitHandle<ManualReset>>();

        AutoResetEvent callbackCanStart(false);
        AutoResetEvent resultEvent(false);
        wh->BeginWaitOne(
            TimeSpan::MaxValue,
            [&resultEvent, &callbackCanStart] (AsyncOperationSPtr const & op) mutable
            {
                Trace.WriteInfo(TraceType, "callback waiting for action");
                callbackCanStart.WaitOne(); 
                Trace.WriteInfo(TraceType, "callback starting action");
                ErrorCode result = op->End(op);
                VERIFY_IS_TRUE(result.IsSuccess());
                resultEvent.Set();
                Trace.WriteInfo(TraceType, "leaving callback");
            },
            AsyncOperationSPtr());

        //make wh destruction and callback race
        wh->Set();
        wh.reset();
        callbackCanStart.Set();

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));
    }

    BOOST_AUTO_TEST_CASE(ManualResetAsyncWaitHandleDestructRaceCallback)
    {
        ENTER;
        AsyncWaitHandleDestructRaceCallback<true>();
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AutoResetAsyncWaitHandleDestructRaceCallback)
    {
        ENTER;
        AsyncWaitHandleDestructRaceCallback<false>();
        LEAVE;
    }

    template <bool ManualReset> void AsyncWaitHandleSetBeforeWait()
    {
        AsyncWaitHandle<ManualReset> asyncWaitHandle(true);

        AutoResetEvent resultEvent(false);
        asyncWaitHandle.BeginWaitOne(
            TimeSpan::MaxValue,
            [&asyncWaitHandle, &resultEvent] (AsyncOperationSPtr const & op)
            {
                ErrorCode result = asyncWaitHandle.EndWaitOne(op);
                VERIFY_IS_TRUE(result.IsSuccess());
                resultEvent.Set();
            },
            AsyncOperationSPtr());

        BOOST_REQUIRE(resultEvent.WaitOne(TimeSpan::FromSeconds(60)));
    }

    BOOST_AUTO_TEST_CASE(ManualResetAsyncWaitHandleSetBeforeWait)
    {
        ENTER;
        AsyncWaitHandleSetBeforeWait<true>();
        LEAVE;
    }

    BOOST_AUTO_TEST_CASE(AutoResetAsyncWaitHandleSetBeforeWait)
    {
        ENTER;
        AsyncWaitHandleSetBeforeWait<false>();
        LEAVE;
    }

    BOOST_AUTO_TEST_SUITE_END()
} // end namespace Common
