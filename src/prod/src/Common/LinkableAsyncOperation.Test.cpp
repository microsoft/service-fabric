// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

Common::StringLiteral LinkableTraceType("LinkableAsyncOperationTest");

namespace Common
{
    using namespace std;


    class AsyncOpContainer : public ComponentRoot
    {
        DENY_COPY(AsyncOpContainer)

    public:
        AsyncOpContainer() 
            : ComponentRoot() 
            , primary_(nullptr)
            , lock_()
            , pendingOps_()
            , allOpsCompletedEvent_()
        {
        }

        virtual ~AsyncOpContainer() {}

        AsyncOperationSPtr BeginOp(
            wstring const & id,
            TimeSpan const timeToWait,
            TimeSpan const timeout,
            bool hasRetryableErrors,
            ErrorCode const expectedEndResult)
        {
            AsyncOperationSPtr operation;
            {
                AcquireExclusiveLock lock(lock_);
                ++pendingOps_;
                if (primary_)
                {
                    Trace.WriteInfo(LinkableTraceType, "Create secondary {0}. {1} pending ops", id, pendingOps_);
                    operation = AsyncOperation::CreateAndStart<MyLinkableAsyncOperation>(
                        *this,
                        id,
                        timeToWait, 
                        timeout,
                        hasRetryableErrors,
                        primary_,
                        [this, expectedEndResult](AsyncOperationSPtr const & op) { OnOpCompleted(op, false, expectedEndResult); },
                        this->CreateAsyncOperationRoot());
                }
                else
                {
                    VERIFY_ARE_EQUAL(pendingOps_, 1, L"Primary is the first operation");
                    Trace.WriteInfo(LinkableTraceType, "Create primary {0}.", id);
                    operation = AsyncOperation::CreateAndStart<MyLinkableAsyncOperation>(
                        *this,
                        id,
                        timeToWait, 
                        timeout,
                        hasRetryableErrors,
                        [this, expectedEndResult](AsyncOperationSPtr const & op) { OnOpCompleted(op, false, expectedEndResult); },
                        this->CreateAsyncOperationRoot());
                    primary_ = operation;
                }
            }

            AsyncOperation::Get<MyLinkableAsyncOperation>(operation)->ResumeOutsideLock(operation);
            OnOpCompleted(operation, true, expectedEndResult);

            return operation;
        }

        void OnOpCompleted(
            AsyncOperationSPtr const& operation, 
            bool expectedCompletedSynchronously,
            ErrorCode const expectedEndResult)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            auto error = MyLinkableAsyncOperation::End(operation);
            Trace.WriteInfo(LinkableTraceType, "Operation completed with {0}, expected {1}", error, expectedEndResult);
            VERIFY_ARE_EQUAL(
                error.ReadValue(), 
                expectedEndResult.ReadValue(), 
                L"Operation completed with expected error");

            {
                AcquireExclusiveLock lock(lock_);
                --pendingOps_;
                if (pendingOps_ == 0)
                {
                    primary_.reset();
                    Trace.WriteInfo(LinkableTraceType, "All operations completed");
                    allOpsCompletedEvent_.Set();
                }
            }
        }

        void WaitForOpsCompleted()
        {
            auto error = allOpsCompletedEvent_.Wait(TimeSpan::FromSeconds(10));
            Trace.WriteInfo(LinkableTraceType, "AllOpsCompleted.waitOne returned {0}", error);
            VERIFY_IS_TRUE(error.IsSuccess(), L"All operations completed");
            VERIFY_ARE_EQUAL(pendingOps_, 0, L"All operations completed");
        }
        
    private:
        class MyLinkableAsyncOperation : public LinkableAsyncOperation
        {
        public:
            MyLinkableAsyncOperation(
                __in AsyncOpContainer & owner,
                wstring const & id,
                TimeSpan const timeToWait,
                TimeSpan const timeout,
                bool hasRetryableErrors,
                AsyncOperationSPtr const & primary,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent)
              : LinkableAsyncOperation(primary, callback, parent)
              , owner_(owner)
              , id_(id)
              , hasRetryableErrors_(hasRetryableErrors)
              , waitEvent_()
              , timeToWait_(timeToWait)
              , timeoutHelper_(timeout)
              , timerActivity_()
              , timerTimeout_()
            {
            }

            MyLinkableAsyncOperation(
                __in AsyncOpContainer & owner,
                wstring const & id,
                TimeSpan const timeToWait,
                TimeSpan const timeout,
                bool hasRetryableErrors,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent)
              : LinkableAsyncOperation(callback, parent)
              , owner_(owner)
              , id_(id)
              , hasRetryableErrors_(hasRetryableErrors)
              , waitEvent_()
              , timeToWait_(timeToWait)
              , timeoutHelper_(timeout)
              , timerActivity_()
              , timerTimeout_()
            {
            }

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation)
            {
                auto casted = AsyncOperation::End<MyLinkableAsyncOperation>(asyncOperation);
                if (casted->timerActivity_)
                {
                    casted->timerActivity_->Cancel();
                    casted->timerActivity_.reset();
                }

                if (casted->timerTimeout_)
                {
                    casted->timerTimeout_->Cancel();
                    casted->timerTimeout_.reset();
                }
                
                return casted->Error;
            }

        protected:
            void OnResumeOutsideLockPrimary(AsyncOperationSPtr const & thisSPtr)
            {
                timerActivity_ = Timer::Create("LinkableAsyncOperationTest1", [this](TimerSPtr const & ) 
                { 
                    Trace.WriteInfo(LinkableTraceType, "Primary {0}: TimerActivity fired", id_);
                    this->waitEvent_.Set(); 
                });
                timerActivity_->Change(timeToWait_);

                timerTimeout_ = Timer::Create("LinkableAsyncOperationTest2", [this, thisSPtr](TimerSPtr const &) 
                { 
                    Trace.WriteInfo(LinkableTraceType, "Primary {0}: TimerTimeout fired", id_);
                    thisSPtr->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Timeout)); 
                });
                timerTimeout_->Change(timeoutHelper_.GetRemainingTime());
        
                Trace.WriteInfo(LinkableTraceType, "Primary {0}: OnResumeOutsideLock", id_);
                auto inner = waitEvent_.BeginWaitOne(
                    timeToWait_.AddWithMaxAndMinValueCheck(TimeSpan::FromMilliseconds(100)), 
                    [this] (AsyncOperationSPtr const & operation) { this->OnWaitCompleted(operation, false); },
                    thisSPtr);
                OnWaitCompleted(inner, true);
            }

            bool IsErrorRetryable(ErrorCode const error)
            {
                bool retry = false;
                if (hasRetryableErrors_ && error.IsError(ErrorCodeValue::Timeout))
                {
                    retry = !timeoutHelper_.IsExpired;
                }

                Trace.WriteInfo(LinkableTraceType, "Error {0} is retryable {1}", error, retry);
                return retry;
            }

        void OnCompleted()
        {
            LinkableAsyncOperation::OnCompleted();
            if (!IsSecondary)
            {
                Trace.WriteInfo(LinkableTraceType, "Primary {0} completed", id_);
                timerActivity_->Cancel();
                timerTimeout_->Cancel();
                if (PromotedPrimary)
                {
                    Trace.WriteInfo(LinkableTraceType, "Old primary {0} completed, but new primary chosen", id_);
                    AcquireExclusiveLock lock(owner_.lock_);
                    owner_.primary_ = this->PromotedPrimary;
                }
            }
        }
        
        private: 
            void OnWaitCompleted(
                AsyncOperationSPtr const & operation, 
                bool expectedCompletedSynchronously)
            {
                if(operation->CompletedSynchronously != expectedCompletedSynchronously)
                {
                    return;
                }

                auto error = waitEvent_.EndWaitOne(operation);
                this->TryComplete(operation->Parent, error);
            }

            AsyncOpContainer & owner_;
            wstring id_;
            bool hasRetryableErrors_;
            TimeoutHelper timeoutHelper_;
            TimeSpan timeToWait_;
            AsyncManualResetEvent waitEvent_;
            TimerSPtr timerActivity_;
            TimerSPtr timerTimeout_;
        };  
        typedef shared_ptr<MyLinkableAsyncOperation> MyLinkableAsyncOperationSPtr;
        
        AsyncOperationSPtr primary_;
        ExclusiveLock lock_;
        int pendingOps_;
        AsyncManualResetEvent allOpsCompletedEvent_;
    };

    class TestLinkableAsyncOperation
    {
    };

    // Start 2 operations - primary and secondary
    // The primary finishes with success, so the secondary must finish with success also
    BOOST_FIXTURE_TEST_SUITE(TestLinkableAsyncOperationSuite,TestLinkableAsyncOperation)

    BOOST_AUTO_TEST_CASE(TestSuccess)
    {
        shared_ptr<AsyncOpContainer> root = make_shared<AsyncOpContainer>();
        root->BeginOp(
            L"TestSuccess0",
            TimeSpan::FromMilliseconds(100),
            TimeSpan::FromSeconds(1),
            false,
            ErrorCode(ErrorCodeValue::Success));

        root->BeginOp(
            L"TestSuccess1",
            TimeSpan::FromSeconds(10),
            TimeSpan::FromSeconds(1),
            false,
            ErrorCode(ErrorCodeValue::Success));

        root->WaitForOpsCompleted();
    }

    // Start 2 operations - primary and secondary
    // The primary finishes with Timeout, so the secondary must finish with Timeout also
    BOOST_AUTO_TEST_CASE(TestTimeoutNoRetryableErrors)
    {
        shared_ptr<AsyncOpContainer> root = make_shared<AsyncOpContainer>();
        root->BeginOp(
            L"TestTimeoutNoRetryableErrors0",
            TimeSpan::FromSeconds(1),
            TimeSpan::FromMilliseconds(100),
            false,
            ErrorCode(ErrorCodeValue::Timeout));

        root->BeginOp(
            L"TestTimeoutNoRetryableErrors1",
            TimeSpan::FromMilliseconds(100),
            TimeSpan::FromSeconds(1),
            false,
            ErrorCode(ErrorCodeValue::Timeout));

        root->WaitForOpsCompleted();
    }

    // Start 3 operations - primary, S1 and S2
    // The primary finishes with Timeout, 
    // so the first secondary is promoted to primary
    // It completes with success, so the third secondary also succeeds
    BOOST_AUTO_TEST_CASE(TestTimeoutWithRetryableErrors)
    {
        shared_ptr<AsyncOpContainer> root = make_shared<AsyncOpContainer>();
        // Wait time is longer than timeout, so op times out
        // Before timing out, the other operations are linked to it as secondaries
        root->BeginOp(
            L"TestTimeoutWithRetryableErrors0",
            TimeSpan::FromSeconds(1),
            TimeSpan::FromMilliseconds(100),
            true,
            ErrorCode(ErrorCodeValue::Timeout));

        root->BeginOp(
            L"TestTimeoutWithRetryableErrors1",
            TimeSpan::FromMilliseconds(100),
            TimeSpan::FromSeconds(1),
            true,
            ErrorCode(ErrorCodeValue::Success));

        root->BeginOp(
            L"TestTimeoutWithRetryableErrors2",
            TimeSpan::FromMilliseconds(500),
            TimeSpan::FromSeconds(1),
            true,
            ErrorCode(ErrorCodeValue::Success));
        root->WaitForOpsCompleted();
    }

    // Start 3 operations: primary and 2 secondaries,
    // with secondary timeouts larger than primary timeout.
    // Cancel the first secondary - make sure it completes with the correct error.
    // When primary completes, the second secondary should be promoted to primary.
    BOOST_AUTO_TEST_CASE(TestCancelSecondary)
    {
        shared_ptr<AsyncOpContainer> root = make_shared<AsyncOpContainer>();
        // Wait time is longer than timeout, so op times out
        // Before timing out, the other operations are linked to it as secondaries
        root->BeginOp(
            L"TestCancelSecondary0",
            TimeSpan::FromSeconds(10),
            TimeSpan::FromSeconds(2),
            true,
            ErrorCode(ErrorCodeValue::Timeout));

        auto secondary = root->BeginOp(
            L"TestCancelSecondary1",
            TimeSpan::FromSeconds(5),
            TimeSpan::FromSeconds(7),
            true,
            ErrorCode(ErrorCodeValue::OperationCanceled));
        secondary->Cancel(true);

        root->BeginOp(
            L"TestCancelSecondary2",
            TimeSpan::FromSeconds(1),
            TimeSpan::FromSeconds(5),
            true,
            ErrorCode(ErrorCodeValue::Success));

        root->WaitForOpsCompleted();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
