// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    class LockTestAsyncOperation : public AsyncOperation
    {
    public:
        LockTestAsyncOperation(
            __in AsyncExclusiveLock & lock,
            __in uint64 & number,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , lock_(lock)
        , number_(number)
        , timeout_(timeout)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation, __in int & count)
        {
            auto casted = AsyncOperation::End<LockTestAsyncOperation>(operation);
            ++count;
            if (casted->Error.IsSuccess())
            {
                casted->lock_.Release(operation);
            }

            return casted->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            lock_.BeginAcquire(
                timeout_, 
                [this] (AsyncOperationSPtr const & lockOperation)
                {
                    OnLockAcquired(lockOperation);
                },
                thisSPtr);
        }

    private:
        void OnLockAcquired(AsyncOperationSPtr const & lockOperation)
        {        
            ErrorCode error = lock_.EndAcquire(lockOperation);
            if (error.IsSuccess())
            {
                // force this to go on a different thread
                Threadpool::Post(
                    [this, lockOperation]()
                    {
                        uint64 numberVal = number_;
                        Sleep(50);
                        number_ = numberVal + number_;
                        Sleep(50);
                        number_ = numberVal + number_;
                        Sleep(50);
                        Trace.WriteInfo("Common::Test", "number is {0}; should be {1}", number_, numberVal * 3L);
                        VERIFY_ARE_EQUAL2(numberVal * 3L, number_);
                        TryComplete(lockOperation->Parent, ErrorCode(ErrorCodeValue::Success));
                    });
            }
            else
            {
                TryComplete(lockOperation->Parent, error);
            }
        }

        AsyncExclusiveLock & lock_;
        uint64 & number_;
        TimeSpan timeout_;
    };

    class LockTestSleeperAsyncOperation : public AsyncOperation
    {
    public:
        LockTestSleeperAsyncOperation(
            __in AsyncExclusiveLock & lock,
            TimeSpan napTime,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , lock_(lock)
        , napTime_(napTime)
        , timeout_(timeout)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation, __in int & count)
        {
            auto casted = AsyncOperation::End<LockTestSleeperAsyncOperation>(operation);
            ++count;
            casted->lock_.Release(operation);
            return casted->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            lock_.BeginAcquire(
                timeout_, 
                [this] (AsyncOperationSPtr const & lockOperation)
                {
                    OnLockAcquired(lockOperation);
                },
                thisSPtr);
        }

    private:
        void OnLockAcquired(AsyncOperationSPtr const & lockOperation)
        {        
            ErrorCode error = lock_.EndAcquire(lockOperation);
            if (error.IsSuccess())
            {
                // force this to go on a different thread
                Threadpool::Post(
                    [this, lockOperation]()
                    {
                        Sleep(static_cast<DWORD>(napTime_.TotalMilliseconds()));
                        TryComplete(lockOperation->Parent, ErrorCode(ErrorCodeValue::Success));
                    });
            }
            else
            {
                TryComplete(lockOperation->Parent, error);
            }
        }

        AsyncExclusiveLock & lock_;
        TimeSpan napTime_;
        TimeSpan timeout_;
    };

    class TestRoot : public ComponentRoot
    {
    };

    BOOST_AUTO_TEST_SUITE(AsyncExclusiveLockTest)

    BOOST_AUTO_TEST_CASE(StressTest)
    {
        shared_ptr<TestRoot> root = make_shared<TestRoot>();
        int runs = 25;
        uint64 number = 5;
        int count = 0;
        AsyncExclusiveLock asyncLock;
        AutoResetEvent reset(false);

        for (int i = 0; i < runs; ++i)
        {
            AsyncOperation::CreateAndStart<LockTestAsyncOperation>(
                asyncLock,
                number,
                TimeSpan::FromSeconds(19),
                [this, &count, &reset] (AsyncOperationSPtr const & testOperation)
                {
                    VERIFY_IS_TRUE(LockTestAsyncOperation::End(testOperation, count).IsSuccess());
                    reset.Set();
                },
                root->CreateAsyncOperationRoot());
        }

        while (count < runs)
        {
            VERIFY_IS_TRUE(reset.WaitOne(TimeSpan::FromSeconds(20)));
        }

        uint64 result = static_cast<uint64>(std::pow(3.0, runs)) * 5L;
        VERIFY_ARE_EQUAL2(result, number);
    }

    BOOST_AUTO_TEST_CASE(TimeoutAfterFirstOperationTest)
    {
        shared_ptr<TestRoot> root = make_shared<TestRoot>();
        int attempts = 5;
        uint64 number = 5;
        int count = 0;
        AsyncExclusiveLock asyncLock;
        AutoResetEvent reset(false);

        AsyncOperation::CreateAndStart<LockTestSleeperAsyncOperation>(
            asyncLock,
            TimeSpan::FromSeconds(2),
            TimeSpan::FromSeconds(5),
            [this, &count, &reset] (AsyncOperationSPtr const & testOperation)
            {
                VERIFY_IS_TRUE(LockTestSleeperAsyncOperation::End(testOperation, count).IsSuccess());
                reset.Set();
            },
            root->CreateAsyncOperationRoot());

        for (int i = 0; i < attempts; ++i)
        {
            AsyncOperation::CreateAndStart<LockTestAsyncOperation>(
                asyncLock,
                number,
                TimeSpan::FromMilliseconds(10),
                [this, &count, &reset] (AsyncOperationSPtr const & testOperation)
                {
                    VERIFY_IS_TRUE(LockTestAsyncOperation::End(testOperation, count).IsError(ErrorCodeValue::Timeout));
                    reset.Set();
                },
                root->CreateAsyncOperationRoot());
        }

        while (count < attempts + 1)
        {
            VERIFY_IS_TRUE(reset.WaitOne(TimeSpan::FromSeconds(20)));
        }

        VERIFY_ARE_EQUAL2(5, number);

        // make sure lock isn't stuck
        AsyncOperation::CreateAndStart<LockTestAsyncOperation>(
            asyncLock,
            number,
            TimeSpan::FromSeconds(2),
            [this, &count, &reset] (AsyncOperationSPtr const & testOperation)
            {
                VERIFY_IS_TRUE(LockTestAsyncOperation::End(testOperation, count).IsSuccess());
                reset.Set();
            },
            root->CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(reset.WaitOne(TimeSpan::FromSeconds(5)));
        VERIFY_ARE_EQUAL2(15, number);
    }

    BOOST_AUTO_TEST_CASE(TryTimeoutFirstOperationTest)
    {
        shared_ptr<TestRoot> root = make_shared<TestRoot>();
        uint64 number = 5;
        int count = 0;
        AsyncExclusiveLock asyncLock;
        AutoResetEvent reset(false);

        AsyncOperation::CreateAndStart<LockTestSleeperAsyncOperation>(
            asyncLock,
            TimeSpan::FromSeconds(2),
            TimeSpan::FromMilliseconds(1),
            [this, &count, &reset] (AsyncOperationSPtr const & testOperation)
            {
                VERIFY_IS_TRUE(LockTestSleeperAsyncOperation::End(testOperation, count).IsSuccess());
                reset.Set();
            },
            root->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(reset.WaitOne(TimeSpan::FromSeconds(20)));

        // make sure lock isn't stuck
        AsyncOperation::CreateAndStart<LockTestAsyncOperation>(
            asyncLock,
            number,
            TimeSpan::FromSeconds(2),
            [this, &count, &reset] (AsyncOperationSPtr const & testOperation)
            {
                VERIFY_IS_TRUE(LockTestAsyncOperation::End(testOperation, count).IsSuccess());
                reset.Set();
            },
            root->CreateAsyncOperationRoot());
        VERIFY_IS_TRUE(reset.WaitOne(TimeSpan::FromSeconds(5)));
        VERIFY_ARE_EQUAL2(15, number);
    }

    BOOST_AUTO_TEST_CASE(TimeoutLeakTest)
    {
        auto root = make_shared<ComponentRoot>();
        AsyncExclusiveLock asyncLock;

        VERIFY_IS_TRUE(asyncLock.Test_GetListSize() == 0);
        
        ManualResetEvent event(false);

        // Acquire lock
        //
        auto operation = asyncLock.BeginAcquire(
            TimeSpan::FromSeconds(5), 
            [&event](AsyncOperationSPtr const &) { event.Set(); },
            root->CreateAsyncOperationRoot());

        event.WaitOne();

        auto error = asyncLock.EndAcquire(operation);
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(asyncLock.Test_GetListSize() == 1);

        // Timeout waiting on lock
        //
        event.Reset();

        operation = asyncLock.BeginAcquire(
            TimeSpan::FromSeconds(5), 
            [&event](AsyncOperationSPtr const &) { event.Set(); },
            root->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(asyncLock.Test_GetListSize() == 2);

        event.WaitOne();

        error = asyncLock.EndAcquire(operation);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Timeout));

        // The timed out waiter should be released from the waiting list
        //
        VERIFY_IS_TRUE(asyncLock.Test_GetListSize() == 1);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
