// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <Common/ErrorCode.h>

Common::StringLiteral const TraceType("ErrorCodeTest");

using namespace std;

namespace Common
{
    struct LoopbackStream : public ByteStream<LoopbackStream>
    {
        void WriteBytes(void const * buf, size_t size) 
        {
            Trace.WriteInfo(TraceType, "writing {0} bytes", size); 
            byte const * begin = reinterpret_cast<byte const*>(buf);
            q_.insert(q_.end(), begin, begin + size);
        }
        void ReadBytes(void * buf, size_t size) 
        {
            Trace.WriteInfo(TraceType, "reading {0} bytes", size); 
            byte * begin = reinterpret_cast<byte*>(buf);
            q_.pop_front_n(size, begin);
        }
        bool empty() { return q_.empty(); }
    private:
        bique<byte> q_;
    };

    class TestErrorCode
    {
    protected:
        ErrorCode GetErrorCode()
        {
            return ErrorCode();
        }
    };

    BOOST_FIXTURE_TEST_SUITE(ErrorCodeTests,TestErrorCode)

    BOOST_AUTO_TEST_CASE(BasicErrorCodeTest)
    {
        ErrorCode successErrorCode;
        VERIFY_IS_TRUE(successErrorCode.IsSuccess());

        ErrorCode timeoutErrorCode(ErrorCodeValue::Timeout);
        VERIFY_IS_FALSE(timeoutErrorCode.IsSuccess());

        ErrorCode faultErrorCode(ErrorCodeValue::RoutingNodeDoesNotMatchFault);
        VERIFY_IS_FALSE(faultErrorCode.IsSuccess());

        // Ensure that we have not used all reserved HRESULT values.
        auto firstInternal = static_cast<int>(ErrorCodeValue::FIRST_INTERNAL_ERROR_CODE_MINUS_ONE) + 1;
        auto lastPublic = static_cast<int>(::FABRIC_E_LAST_USED_HRESULT);
        VERIFY_IS_TRUE(lastPublic < firstInternal);

        // Ensure that internal HRESULT values are at the end of our reserved range.
        auto lastInternal = static_cast<int>(ErrorCodeValue::LAST_INTERNAL_ERROR_CODE);
        auto lastReserved = static_cast<int>(FABRIC_E_LAST_RESERVED_HRESULT);
        if (lastInternal != lastReserved)
        {
            auto actualCount = (lastInternal + 1) - firstInternal;
            auto specifiedCount = static_cast<int>(ErrorCodeValue::INTERNAL_ERROR_CODE_COUNT);

            Trace.WriteError(
                TraceType,
                "ErrorCodeValue::INTERNAL_ERROR_CODE_COUNT is {0} but should be {1}.  Please fix it before checking in.",
                specifiedCount,
                actualCount);

            VERIFY_FAIL(wformatString(
                    "ErrorCodeValue::INTERNAL_ERROR_CODE_COUNT is %d but should be %d.  Please fix it before checking in.",
                    specifiedCount,
                    actualCount).c_str());
        }
    }

    BOOST_AUTO_TEST_CASE(TransferErrorCodeTest)
    {
        Trace.WriteInfo(TraceType, "Copy via assignment -- error1A should end up as 'read'");
        ErrorCode error1A;
        ErrorCode error2A = error1A;
        VERIFY_IS_TRUE(error2A.IsSuccess());

        Trace.WriteInfo(TraceType, "Copy via constructor -- error1B should end up as 'read'");
        ErrorCode error1B;
        ErrorCode error2B(error1B);
        VERIFY_IS_TRUE(error2B.IsSuccess());

        Trace.WriteInfo(TraceType, "Copy from function call -- inside of function, should end up as 'read'");
        ErrorCode fromFunctionCall = GetErrorCode();
        VERIFY_IS_TRUE(fromFunctionCall.IsSuccess());
    }

    BOOST_AUTO_TEST_CASE(UnreadErrorCodeTest)
    {
        Trace.WriteInfo(TraceType, "Verify overwriting empty ErrorCode does not throw, but then subsequent overwrite throws.");
        ErrorCode error2;
        error2 = ErrorCode::Success();

        {
            Assert::DisableDebugBreakInThisScope disableDebugBreakInThisScope;
            VERIFY_THROWS({ error2 = ErrorCode::Success(); }, std::system_error);
        }

        error2.ReadValue();
    }

    BOOST_AUTO_TEST_CASE(InternalErrorCodePositionTest)
    {
        // ensure that the value of the following internal error code does not change
        // as the new internal error code are added at the top of the internal error code range
        // see comment in ErrorCodeValue.h

        auto lastInternal = static_cast<int>(ErrorCodeValue::LAST_INTERNAL_ERROR_CODE);
        auto abandonedFileWriteLockFoundError = static_cast<int>(ErrorCodeValue::AbandonedFileWriteLockFound);
        
        if (lastInternal != abandonedFileWriteLockFoundError)
        {
              Trace.WriteError(
                TraceType,
                "ErrorCodeValue::LAST_INTERNAL_ERROR_CODE MUST BE AbandonedFileWriteLockFound to maintain compatibility. Please add internal error code at the top of the INTERNAL error code range.");
        }
    }

    BOOST_AUTO_TEST_CASE(ThreadErrorMessageTest)
    {
        LONG threadCount = 200;

        int minSleep = 0;
        int maxSleep = 50;

        atomic_long completedCount(threadCount);
        ManualResetEvent completedEvent(false);

        Random rand(static_cast<int>(DateTime::Now().Ticks));

        for (auto ix=0; ix<threadCount; ++ix)
        {
            LONG index = ix;
            Threadpool::Post([&, index]()
            {
                auto inData = wformatString("error message {0}", index);

                Sleep(rand.Next(minSleep, maxSleep));

                {
                    ErrorCode error(ErrorCodeValue::OperationFailed, wformatString(inData));

                    error.SetThreadErrorMessage();

                    Sleep(rand.Next(minSleep, maxSleep));

                    wstring outData1 = ErrorCode::GetThreadErrorMessage();

                    VERIFY_IS_TRUE(outData1 == inData, wformatString("[{0}] read '{1}'", index, outData1).c_str());
                }

                Sleep(rand.Next(minSleep, maxSleep));

                wstring outData2 = ErrorCode::GetThreadErrorMessage();

                VERIFY_IS_TRUE(outData2.empty(), wformatString("[{0}] read '{1}'", index, outData2).c_str());

                if (--completedCount == 0)
                {
                    completedEvent.Set();
                }
            });
        }

        completedEvent.WaitOne();

        // test copy
        {
            wstring msg(L"my error message 1");
            ErrorCode error1(ErrorCodeValue::OperationFailed, wformatString(msg));

            ErrorCode error2(error1);
            VERIFY_IS_TRUE(error1.Message == error2.Message, wformatString("{0} == {1}", error1.Message, error2.Message).c_str());
            VERIFY_IS_TRUE(error2.Message == msg);
            VERIFY_IS_TRUE(error2.IsError(ErrorCodeValue::OperationFailed));

            ErrorCode error3 = error1;
            VERIFY_IS_TRUE(error1.Message == error3.Message, wformatString("{0} == {1}", error1.Message, error3.Message).c_str());
            VERIFY_IS_TRUE(error3.Message == msg);
            VERIFY_IS_TRUE(error3.IsError(ErrorCodeValue::OperationFailed));

            ErrorCode error4;
            error4.Overwrite(error1);
            VERIFY_IS_TRUE(error1.Message == error4.Message, wformatString("{0} == {1}", error1.Message, error4.Message).c_str());
            VERIFY_IS_TRUE(error4.Message == msg);
            VERIFY_IS_TRUE(error4.IsError(ErrorCodeValue::OperationFailed));
        }

        // test move
        {
            wstring msg(L"my error message 2");
            ErrorCode error1(ErrorCodeValue::OperationFailed, wformatString(msg));

            ErrorCode error2(move(error1));
            VERIFY_IS_TRUE(error1.Message != error2.Message, wformatString("{0} != {1}", error1.Message, error2.Message).c_str());
            VERIFY_IS_TRUE(error1.Message.empty());
            VERIFY_IS_TRUE(error2.Message == msg);
            VERIFY_IS_TRUE(error2.IsError(ErrorCodeValue::OperationFailed));

            ErrorCode error3 = move(error2);
            VERIFY_IS_TRUE(error2.Message != error3.Message, wformatString("{0} != {1}", error2.Message, error3.Message).c_str());
            VERIFY_IS_TRUE(error2.Message.empty());
            VERIFY_IS_TRUE(error3.Message == msg);
            VERIFY_IS_TRUE(error3.IsError(ErrorCodeValue::OperationFailed));

            ErrorCode error4;
            error4.Overwrite(move(error3));
            VERIFY_IS_TRUE(error3.Message != error4.Message, wformatString("{0} != {1}", error3.Message, error4.Message).c_str());
            VERIFY_IS_TRUE(error3.Message.empty());
            VERIFY_IS_TRUE(error4.Message == msg);
            VERIFY_IS_TRUE(error4.IsError(ErrorCodeValue::OperationFailed));
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    class TestFirstErrorTracker
    {};

    BOOST_FIXTURE_TEST_SUITE(FirstErrorTrackerTests, TestFirstErrorTracker)

    BOOST_AUTO_TEST_CASE(BasicFirstErrorTrackerTest)
    {
        FirstErrorTracker errorTracker;
        VERIFY_IS_TRUE(errorTracker.ReadValue() == ErrorCodeValue::Success);

        errorTracker.Update(ErrorCodeValue::Success);
        VERIFY_IS_TRUE(errorTracker.ReadValue() == ErrorCodeValue::Success);

        errorTracker.Update(ErrorCodeValue::StaleRequest);
        VERIFY_IS_TRUE(errorTracker.ReadValue() == ErrorCodeValue::StaleRequest);

        errorTracker.Update(ErrorCodeValue::Success);
        VERIFY_IS_TRUE(errorTracker.ReadValue() == ErrorCodeValue::StaleRequest);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
