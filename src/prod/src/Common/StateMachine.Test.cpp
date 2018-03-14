// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;

Common::StringLiteral const TraceType("StateMachineTest");

namespace Common
{
    class MyStateMachine : public StateMachine, public ComponentRoot
    {
        DENY_COPY(MyStateMachine)

        STATEMACHINE_STATES_07(Created, Opening, Opened, Modifying, Closing, Closed, Failed)
        STATEMACHINE_REF_COUNTED_STATES(Modifying)

        STATEMACHINE_TRANSITION(Opening, Created);
        STATEMACHINE_TRANSITION(Opened, Opening|Modifying);
        STATEMACHINE_TRANSITION(Modifying, Opened|Modifying);
        STATEMACHINE_TRANSITION(Failed, Opening|Closing);
        STATEMACHINE_TRANSITION(Closing, Opened);
        STATEMACHINE_TRANSITION(Closed, Closing);
        STATEMACHINE_ABORTED_TRANSITION(Created|Opened|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Closed);
        
    public:
        MyStateMachine();
        virtual ~MyStateMachine();

        AsyncOperationSPtr BeginOpen(
            TimeSpan const completeAfter, 
            bool failOperation,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        ErrorCode EndOpen(AsyncOperationSPtr const & operation);

        AsyncOperationSPtr BeginModify(
            TimeSpan const completeAfter, 
            bool failOperation,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        ErrorCode EndModify(AsyncOperationSPtr const & operation);

        AsyncOperationSPtr BeginClose(
            TimeSpan const completeAfter, 
            bool failOperation,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        void EndClose(AsyncOperationSPtr const & operation);

        // Test helper methods ref counting test.
        void TransitionIntoModifying(int iterCount);
        void TransitionOutOfModifying(int iterCount);

    protected:
        void OnAbort();

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class ModifyAsyncOperation;
    };
    
    
    MyStateMachine::MyStateMachine()
        : StateMachine(Created), 
        ComponentRoot()
    {
    }

    MyStateMachine::~MyStateMachine()
    {
    }

    class MyStateMachine::OpenAsyncOperation : public AsyncOperation
    {
        DENY_COPY(OpenAsyncOperation)
    public:
        OpenAsyncOperation(
            __in MyStateMachine & owner,
            TimeSpan const completeAfter,
            bool failOperation,
            AsyncCallback const &callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            completeAfter_(completeAfter),
            failOperation_(failOperation),
            completionTimer_()
        {
        }

        virtual ~OpenAsyncOperation()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            auto error = owner_.TransitionToOpening();
            if (!error.IsSuccess())
            {
                if (owner_.GetState() == MyStateMachine::Opened)
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                    return;
                }
                else
                {
                    TryComplete(thisSPtr, error);
                    return;
                }
            }

            if (completeAfter_ == TimeSpan::Zero)
            {
                CompleteOperation(thisSPtr);
                return;
            }
            else
            {
                completionTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer) { this->OnCompletionTimerCallback(timer, thisSPtr); });
                completionTimer_->Change(completeAfter_);
            }
        }

    private:
        void OnCompletionTimerCallback(TimerSPtr const & timer, AsyncOperationSPtr const & thisSPtr)
        {
            timer->Cancel();
            CompleteOperation(thisSPtr);
        }

        void CompleteOperation(AsyncOperationSPtr const & thisSPtr)
        {
            if (failOperation_)
            {
                owner_.TransitionToFailed().ReadValue();
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                return;
            }
            else
            {
                auto error = owner_.TransitionToOpened();
                if (!error.IsSuccess())
                {
                    owner_.TransitionToFailed().ReadValue();
                    TryComplete(thisSPtr, error);
                    return;
                }
                else
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                    return;
                }
            }
        }

    private:
        MyStateMachine & owner_;
        bool const failOperation_;
        TimeSpan const completeAfter_;
        TimerSPtr completionTimer_;
    };

    class MyStateMachine::ModifyAsyncOperation : public AsyncOperation
    {
        DENY_COPY(ModifyAsyncOperation)
    public:
        ModifyAsyncOperation(
            __in MyStateMachine & owner,
            TimeSpan const completeAfter,
            bool failOperation,
            AsyncCallback const &callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            completeAfter_(completeAfter),
            failOperation_(failOperation),
            completionTimer_()
        {
        }

        virtual ~ModifyAsyncOperation()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<ModifyAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            auto error = owner_.TransitionToModifying();
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }

            if (completeAfter_ == TimeSpan::Zero)
            {
                CompleteOperation(thisSPtr);
                return;
            }
            else
            {
                completionTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer) { this->OnCompletionTimerCallback(timer, thisSPtr); });
                completionTimer_->Change(completeAfter_);
            }
        }

    private:
        void OnCompletionTimerCallback(TimerSPtr const & timer, AsyncOperationSPtr const & thisSPtr)
        {
            timer->Cancel();
            CompleteOperation(thisSPtr);
        }

        void CompleteOperation(AsyncOperationSPtr const & thisSPtr)
        {
            if (failOperation_)
            {
                owner_.TransitionToFailed().ReadValue();
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                return;
            }
            else
            {
                auto error = owner_.TransitionToOpened();
                if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::OperationsPending))
                {
                    owner_.TransitionToFailed().ReadValue();
                    TryComplete(thisSPtr, error);
                    return;
                }
                else
                {
                    TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                    return;
                }
            }
        }

    private:
        MyStateMachine & owner_;
        bool const failOperation_;
        TimeSpan const completeAfter_;
        TimerSPtr completionTimer_;
    };
        
    class MyStateMachine::CloseAsyncOperation : public AsyncOperation
    {
        DENY_COPY(CloseAsyncOperation)
    public:
        CloseAsyncOperation(
            __in MyStateMachine & owner,
            TimeSpan const completeAfter,
            bool failOperation,
            AsyncCallback const &callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
            owner_(owner),
            completeAfter_(completeAfter),
            failOperation_(failOperation),
            completionTimer_()
        {
        }

        virtual ~CloseAsyncOperation()
        {
        }

        static void End(AsyncOperationSPtr const & operation)
        {
            auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
            thisPtr->Error.ReadValue();
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            if (owner_.GetState() == MyStateMachine::Closed)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                return;
            }

            auto error = owner_.TransitionToClosing();
            if (!error.IsSuccess())
            {
                TransitionToAborted(thisSPtr);
                return;
            }

            if (completeAfter_ == TimeSpan::Zero)
            {
                CompleteOperation(thisSPtr);
                return;
            }
            else
            {
                completionTimer_ = Timer::Create(__FUNCTION__, [this, thisSPtr](TimerSPtr const & timer) { this->OnCompletionTimerCallback(timer, thisSPtr); });
                completionTimer_->Change(completeAfter_);
            }
        }

    private:
        void OnCompletionTimerCallback(TimerSPtr const & timer, AsyncOperationSPtr const & thisSPtr)
        {
            timer->Cancel();
            CompleteOperation(thisSPtr);
        }

        void CompleteOperation(AsyncOperationSPtr const & thisSPtr)
        {
            if (failOperation_)
            {
                TransitionToFailed(thisSPtr);
            }
            else
            {
                TransitionToClosed(thisSPtr);
            }
        }

        void TransitionToClosed(AsyncOperationSPtr const & thisSPtr)
        {
            auto error = owner_.TransitionToClosed();
            if (!error.IsSuccess())
            {
                TransitionToFailed(thisSPtr);
                return;
            }
            else
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                return;
            }
        }

        void TransitionToFailed(AsyncOperationSPtr const & thisSPtr)
        {
            owner_.TransitionToFailed().ReadValue();
            TransitionToAborted(thisSPtr);
        }

        void TransitionToAborted(AsyncOperationSPtr const & thisSPtr)
        {
            owner_.AbortAndWaitForTerminationAsync(
                [this](AsyncOperationSPtr const & operation)
                {
                    this->TryComplete(operation->Parent, ErrorCode(ErrorCodeValue::Success));
                },
            thisSPtr);
        }

    private:
        MyStateMachine & owner_;
        bool const failOperation_;
        TimeSpan const completeAfter_;
        TimerSPtr completionTimer_;
    };

    AsyncOperationSPtr MyStateMachine::BeginOpen(
        TimeSpan const completeAfter, 
        bool failOperation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
            *this,
            completeAfter,
            failOperation,
            callback,
            parent);
    }

    ErrorCode MyStateMachine::EndOpen(AsyncOperationSPtr const & operation)
    {
        return OpenAsyncOperation::End(operation);
    }

    AsyncOperationSPtr MyStateMachine::BeginModify(
        TimeSpan const completeAfter, 
        bool failOperation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ModifyAsyncOperation>(
            *this,
            completeAfter,
            failOperation,
            callback,
            parent);
    }

    ErrorCode MyStateMachine::EndModify(AsyncOperationSPtr const & operation)
    {
        return ModifyAsyncOperation::End(operation);
    }

    AsyncOperationSPtr MyStateMachine::BeginClose(
        TimeSpan const completeAfter, 
        bool failOperation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
            *this,
            completeAfter,
            failOperation,
            callback,
            parent);
    }

    void MyStateMachine::EndClose(AsyncOperationSPtr const & operation)
    {
        CloseAsyncOperation::End(operation);
    }

    void MyStateMachine::OnAbort()
    {
    }

    void MyStateMachine::TransitionIntoModifying(int iterCount)
    {
        for (int i = 1; i <= iterCount; i++)
        {
            auto error = this->TransitionToModifying();
            VERIFY_IS_TRUE(error.IsSuccess());

            uint64 state, refCount;
            this->GetStateAndRefCount(state, refCount);

            VERIFY_IS_TRUE(state == MyStateMachine::Modifying);
            VERIFY_IS_TRUE(refCount == i);
        }
    }

    void MyStateMachine::TransitionOutOfModifying(int iterCount)
    {
        for (int i = (iterCount - 1); i >= 0; i--)
        {
            auto error = this->TransitionToOpened();

            auto expectedError = (i == 0) ? ErrorCodeValue::Success : ErrorCodeValue::OperationsPending;

            VERIFY_IS_TRUE(error.IsError(expectedError));

            uint64 state, refCount;
            this->GetStateAndRefCount(state, refCount);

            auto expectedState = (i == 0) ? MyStateMachine::Opened : MyStateMachine::Modifying;

            VERIFY_IS_TRUE(state == expectedState);
            VERIFY_IS_TRUE(refCount == i);
        }
    }

    class StateMachineTest
    {
        //TEST_METHOD(BasicTransitionTest)
        //TEST_METHOD(DirectAbortTest)
    };

    BOOST_FIXTURE_TEST_SUITE(StateMachineTestSuite,StateMachineTest)

    BOOST_AUTO_TEST_CASE(DelayedAbortTest)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();
        auto waiter = make_shared<AsyncOperationWaiter>();

        stateMachine->BeginOpen(
            TimeSpan::FromSeconds(5), 
            false,
            [stateMachine, waiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                waiter->SetError(error);
                waiter->Set();
            },
            stateMachine->CreateAsyncOperationRoot());
        
        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opening); 
        
        stateMachine->Abort();
        waiter->WaitOne();
        
        Trace.WriteNoise(TraceType, "waiter->GetError() = {0}", waiter->GetError());
        VERIFY_IS_TRUE(waiter->GetError().IsError(ErrorCodeValue::ObjectClosed)); 

        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Aborted); 
    }

    BOOST_AUTO_TEST_CASE(AbortWaitTest)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();

        stateMachine->BeginOpen(
            TimeSpan::FromSeconds(5), 
            false,
            [stateMachine](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                error.ReadValue();
            },
            stateMachine->CreateAsyncOperationRoot());
        
        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opening); 
        
        stateMachine->AbortAndWaitForTermination();
        
        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Aborted); 
    }
    
    BOOST_AUTO_TEST_CASE(TransitionWaitTest)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();

        stateMachine->BeginOpen(
            TimeSpan::FromSeconds(5), 
            false,
            [stateMachine](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                error.ReadValue();
            },
            stateMachine->CreateAsyncOperationRoot());
        
        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opening); 
        
        auto waiter = make_shared<AsyncOperationWaiter>();
        stateMachine->BeginWaitForTransition(
            MyStateMachine::Opened,
            [stateMachine, waiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndWaitForTransition(operation);
                waiter->SetError(error);
                waiter->Set();

            },
            stateMachine->CreateAsyncOperationRoot());
        waiter->WaitOne();

        Trace.WriteNoise(TraceType, "waiter->GetError() = {0}", waiter->GetError());
        VERIFY_IS_TRUE(waiter->GetError().IsSuccess()); 

        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opened); 
    }

    BOOST_AUTO_TEST_CASE(TransitionWaitCompletedTest)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();

        stateMachine->BeginOpen(
            TimeSpan::Zero, 
            false,
            [stateMachine](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                error.ReadValue();
            },
            stateMachine->CreateAsyncOperationRoot());
        
        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opened); 
        
        auto waiter = make_shared<AsyncOperationWaiter>();
        stateMachine->BeginWaitForTransition(
            MyStateMachine::Opened,
            [stateMachine, waiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndWaitForTransition(operation);
                waiter->SetError(error);
                waiter->Set();

            },
            stateMachine->CreateAsyncOperationRoot());
        waiter->WaitOne();

        Trace.WriteNoise(TraceType, "waiter->GetError() = {0}", waiter->GetError());
        VERIFY_IS_TRUE(waiter->GetError().IsSuccess()); 

        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opened); 
    }

    BOOST_AUTO_TEST_CASE(TransitionWaitCancelledTest)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();

        stateMachine->BeginOpen(
            TimeSpan::Zero, 
            false,
            [stateMachine](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                error.ReadValue();
            },
            stateMachine->CreateAsyncOperationRoot());
        
        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opened); 
        
        auto waiter = make_shared<AsyncOperationWaiter>();
        stateMachine->BeginWaitForTransition(
            MyStateMachine::Closed,
            [stateMachine, waiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndWaitForTransition(operation);
                waiter->SetError(error);
                waiter->Set();

            },
            stateMachine->CreateAsyncOperationRoot());

        stateMachine->Abort();
        waiter->WaitOne();

        Trace.WriteNoise(TraceType, "waiter->GetError() = {0}", waiter->GetError());
        VERIFY_IS_TRUE(waiter->GetError().IsError(ErrorCodeValue::ObjectClosed)); 

        Trace.WriteNoise(TraceType, "stateMachine->GetState() = {0}", stateMachine->GetState());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Aborted); 
    }

    BOOST_AUTO_TEST_CASE(VerifyRefCounting)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();        
        auto waiter = make_shared<AsyncOperationWaiter>();

        auto error = stateMachine->TransitionToOpening();
        VERIFY_IS_TRUE(error.IsSuccess());

        error = stateMachine->TransitionToOpened();
        VERIFY_IS_TRUE(error.IsSuccess());

        stateMachine->TransitionIntoModifying(5);
        stateMachine->TransitionOutOfModifying(5);

        stateMachine->TransitionIntoModifying(3);
        
        stateMachine->Abort();

        for (int i = 2; i >= 0; i--)
        {
            error = stateMachine->TransitionToOpened();
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ObjectClosed));

            uint64 state, refCount;
            stateMachine->GetStateAndRefCount(state, refCount);

            auto expectedState = (i == 0) ? MyStateMachine::Aborted : MyStateMachine::Modifying;

            VERIFY_IS_TRUE(state == expectedState);
            VERIFY_IS_TRUE(refCount == i);
        }
    }

    BOOST_AUTO_TEST_CASE(ParallelModify)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();
        auto waiter = make_shared<AsyncOperationWaiter>();

        stateMachine->BeginOpen(
            TimeSpan::Zero,
            false,
            [stateMachine, waiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                waiter->SetError(error);
                waiter->Set();
            },
            stateMachine->CreateAsyncOperationRoot());

        waiter->WaitOne();

        VERIFY_IS_TRUE(waiter->GetError().IsSuccess());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opened);

        long operationCount = 300;
        Random randomNum;
		atomic_long stateTransitionFailureCount(0);

        for (long i = 1; i <= operationCount; i++)
        {
            stateMachine->BeginModify(
                TimeSpan::FromSeconds(randomNum.Next(5, 15)),
                false,
                [stateMachine, &stateTransitionFailureCount](AsyncOperationSPtr const & operation)
                {
                    auto error = stateMachine->EndModify(operation);
					if (!error.IsSuccess())
					{
						++stateTransitionFailureCount;
					}
                },
                stateMachine->CreateAsyncOperationRoot());
        }

        uint64 state, refCount;
        stateMachine->GetStateAndRefCount(state, refCount);

        VERIFY_IS_TRUE(state == MyStateMachine::Modifying);
        VERIFY_IS_TRUE(refCount == operationCount);

        auto modifyWaiter = make_shared<AsyncOperationWaiter>();
        stateMachine->BeginWaitForTransition(
            MyStateMachine::Opened,
            [stateMachine, modifyWaiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndWaitForTransition(operation);
                modifyWaiter->SetError(error);
                modifyWaiter->Set();

            },
            stateMachine->CreateAsyncOperationRoot());
        
        modifyWaiter->WaitOne();

        VERIFY_IS_TRUE(waiter->GetError().IsSuccess());

		VERIFY_IS_TRUE(stateTransitionFailureCount.load() == 0);

        uint64 state1, refCount1;
        stateMachine->GetStateAndRefCount(state1, refCount1);
        
        VERIFY_IS_TRUE(refCount1 == 0);
        VERIFY_IS_TRUE(state1 == MyStateMachine::Opened);
    }

    BOOST_AUTO_TEST_CASE(ParallelModifyAbortWait)
    {
        // create state machine
        auto stateMachine = make_shared<MyStateMachine>();
        auto waiter = make_shared<AsyncOperationWaiter>();

        stateMachine->BeginOpen(
            TimeSpan::Zero,
            false,
            [stateMachine, waiter](AsyncOperationSPtr const & operation)
            {
                auto error = stateMachine->EndOpen(operation);
                waiter->SetError(error);
                waiter->Set();
            },
            stateMachine->CreateAsyncOperationRoot());

        waiter->WaitOne();

        VERIFY_IS_TRUE(waiter->GetError().IsSuccess());
        VERIFY_IS_TRUE(stateMachine->GetState() == MyStateMachine::Opened);

        long operationCount = 300;
        Random randomNum;
		atomic_long stateTransitionFailureCount(0);

        for (long i = 1; i <= operationCount; i++)
        {
            stateMachine->BeginModify(
                TimeSpan::FromSeconds(randomNum.Next(5, 15)),
                false,
                [stateMachine, &stateTransitionFailureCount](AsyncOperationSPtr const & operation)
                {
                    auto error = stateMachine->EndModify(operation);

                    if (!error.IsError(ErrorCodeValue::ObjectClosed))
					{
						++stateTransitionFailureCount;
					}

                },
                stateMachine->CreateAsyncOperationRoot());
        }

        uint64 state, refCount;
        stateMachine->GetStateAndRefCount(state, refCount);

        VERIFY_IS_TRUE(state == MyStateMachine::Modifying);
        VERIFY_IS_TRUE(refCount == operationCount);

        stateMachine->AbortAndWaitForTermination();

		VERIFY_IS_TRUE(stateTransitionFailureCount.load() == 0);

        uint64 state1, refCount1;
        stateMachine->GetStateAndRefCount(state1, refCount1);

        VERIFY_IS_TRUE(refCount1 == 0);
        VERIFY_IS_TRUE(state1 == MyStateMachine::Aborted);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
