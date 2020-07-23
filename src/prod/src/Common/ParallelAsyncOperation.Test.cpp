// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

Common::StringLiteral ParallelAsyncTestTraceType("ParallelAsyncOperationTest");

namespace Common
{
    using namespace std;

    class CancelTesterAsyncOperation : public AsyncOperation
    {
    public:
        CancelTesterAsyncOperation(
            wstring const & name,
            std::vector<wstring> const & namesToCompleteSynchronously,
            atomic_uint64 & cancelledAsyncOperationCount,
            TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
            , name_(name)
            , namesToCompleteSynchronously_(namesToCompleteSynchronously)
            , cancelledAsyncOperationCount_(cancelledAsyncOperationCount)
            , timeoutHelper_(timeout)
        {
        }

        static Common::ErrorCode End(AsyncOperationSPtr const & operation, __out bool & result)
        {
            auto thisSPtr = AsyncOperation::End<CancelTesterAsyncOperation>(operation);
            result = true;
            return thisSPtr->Error;
        }

    protected:
        void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
        {
            Trace.WriteInfo(ParallelAsyncTestTraceType, "CancelTesterAsyncOperation::OnStart called for {0}", name_);

            if(std::find(namesToCompleteSynchronously_.begin(), namesToCompleteSynchronously_.end(), name_) != namesToCompleteSynchronously_.end())
            {
                Trace.WriteInfo(ParallelAsyncTestTraceType, "CancelTesterAsyncOperation::Completing async operation synchronously for {0}", name_);
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                return;
            }

            timer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            });
            timer_->Change(timeoutHelper_.GetRemainingTime());
        }

        void OnCancel()
        {
            Trace.WriteInfo(ParallelAsyncTestTraceType, "CancelTesterAsyncOperation::OnCancel called for {0}", name_);
            cancelledAsyncOperationCount_++;
            timer_->Cancel();
        }

    private:
        vector<wstring> namesToCompleteSynchronously_;
        atomic_uint64 & cancelledAsyncOperationCount_;
        TimerSPtr timer_;
        TimeoutHelper timeoutHelper_;
        wstring name_;

    };

    class CanceledChildrenCounterAsyncOperation : public ParallelVectorAsyncOperation<wstring, bool>
    {
    public:
        CanceledChildrenCounterAsyncOperation(
            std::vector<wstring> const & input,
            std::vector<wstring> const & namesToCompleteSynchronously,
            atomic_uint64 & cancelledAsyncOperationCount,
            TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) :
            ParallelVectorAsyncOperation<wstring, bool>(input, callback, parent),
            namesToCompleteSynchronously_(namesToCompleteSynchronously),
            cancelledAsyncOperationCount_(cancelledAsyncOperationCount),
            timeoutHelper_(timeout)
        {
        }

        static Common::ErrorCode End(AsyncOperationSPtr const & operation, __out std::vector<bool> & outputResult)
        {
            return ParallelVectorAsyncOperation::End(operation, outputResult);
        }

    protected:
        Common::AsyncOperationSPtr OnStartOperation(wstring const & input, Common::AsyncCallback const & operation, Common::AsyncOperationSPtr const & parent)
        {
            return AsyncOperation::CreateAndStart<CancelTesterAsyncOperation>(input, namesToCompleteSynchronously_, cancelledAsyncOperationCount_, timeoutHelper_.GetRemainingTime(), operation, parent);
        }

        virtual Common::ErrorCode OnEndOperation(Common::AsyncOperationSPtr const & operation, wstring const & /* unused */, __out bool & output)
        {
            return CancelTesterAsyncOperation::End(operation, output);
        }

    private:
        vector<wstring> namesToCompleteSynchronously_;
        atomic_uint64 & cancelledAsyncOperationCount_;
        TimeoutHelper timeoutHelper_;
    };

    class TestParallelAsyncOperation
    {
    };

    BOOST_FIXTURE_TEST_SUITE(TestParallelAsyncOperationSuite, TestParallelAsyncOperation)

        BOOST_AUTO_TEST_CASE(CancelParallelOperationsTest)
    {
            TimeSpan timeout = TimeSpan::FromSeconds(25);

            int numberOfChildren = 5;
            vector<wstring> names;            
            for(int i = 0; i < numberOfChildren; i++)
            {
                wstring name = wformatString("Child.{0}", i);
                names.push_back(name);                
            }

            atomic_uint64 cancelledAsyncOperationCount;
            ErrorCode error;
            vector<bool> cancelledResult;
            ManualResetEvent opCompleted;
            AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<CanceledChildrenCounterAsyncOperation>(
                names,
                vector<wstring>(), // do not complete any async op synchronously
                cancelledAsyncOperationCount,
                timeout,
                [&opCompleted, &error](AsyncOperationSPtr const& operation)
            {
                vector<bool> cancelledResult;
                error = CanceledChildrenCounterAsyncOperation::End(operation, cancelledResult);
                opCompleted.Set();
            },
                AsyncOperationSPtr());

            operation->Cancel();
            opCompleted.WaitOne(timeout);

            auto actualCancelledChildrenCount = cancelledAsyncOperationCount.load();

            VERIFY_IS_TRUE(actualCancelledChildrenCount == numberOfChildren, wformatString("Validate the number of cancelled children. Expected={0}, Actual={1}", numberOfChildren, actualCancelledChildrenCount).c_str());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::OperationCanceled), wformatString("CanceledChildrenCounterAsyncOperation completed with {0}", error).c_str());
        }

    BOOST_AUTO_TEST_CASE(CancelPartialParallelOperationsTest)
    {
        TimeSpan timeout = TimeSpan::FromSeconds(25);

        int numberOfChildren = 6;
        vector<wstring> names;
        vector<wstring> namesToCompleteSynchronously;
        for(int i = 0; i < numberOfChildren; i++)
        {
            wstring name = wformatString("Child.{0}", i);
            names.push_back(name);
            if(i % 2 == 0)
            {
                namesToCompleteSynchronously.push_back(name);
            }
        }

        atomic_uint64 cancelledAsyncOperationCount;
        ErrorCode error;
        vector<bool> cancelledResult;
        ManualResetEvent opCompleted;
        AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<CanceledChildrenCounterAsyncOperation>(
            names,
            namesToCompleteSynchronously,
            cancelledAsyncOperationCount,
            timeout,
            [&opCompleted, &error](AsyncOperationSPtr const& operation)
        {
            vector<bool> cancelledResult;
            error = CanceledChildrenCounterAsyncOperation::End(operation, cancelledResult);
            opCompleted.Set();
        },
            AsyncOperationSPtr());

        operation->Cancel();
        opCompleted.WaitOne(timeout);

        auto expectedCancelledChildrenCount = numberOfChildren - namesToCompleteSynchronously.size();
        auto actualCancelledChildrenCount = cancelledAsyncOperationCount.load();
        
        VERIFY_IS_TRUE(actualCancelledChildrenCount == expectedCancelledChildrenCount, wformatString("Validate the number of cancelled children. Expected={0}, Actual={1}", expectedCancelledChildrenCount, actualCancelledChildrenCount).c_str());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::OperationCanceled), wformatString("CanceledChildrenCounterAsyncOperation completed with {0}", error).c_str());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
