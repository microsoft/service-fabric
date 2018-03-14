// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <memory>
#include <functional>
#include "Common/WaitHandle.h"
#include "AsyncOperation.h"
#include "CompletedAsyncOperation.h"
#include "ComponentRoot.h"
#include "RootedObject.h"

using namespace std;

Common::StringLiteral const TraceType("AsyncOperationTest");

namespace Common
{
    static bool CancelScenario = false;

    class ComponentC : RootedObject
    {
        DENY_COPY(ComponentC)

    public:

        ComponentC(ComponentRoot const & root) : RootedObject(root) {}

        AsyncOperationSPtr BeginFooC11(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooC11(AsyncOperationSPtr const& fooC11Operation);

        AsyncOperationSPtr BeginFooC12(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooC12(AsyncOperationSPtr const& fooC12Operation);

        AsyncOperationSPtr BeginFooC21(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooC21(AsyncOperationSPtr const& fooC21Operation);

        AsyncOperationSPtr BeginFooC22(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooC22(AsyncOperationSPtr const& fooC22Operation);
    };

    class ComponentB : RootedObject
    {
        DENY_COPY(ComponentB)

    public:
        ComponentB(ComponentRoot const & root) 
            : RootedObject(root),
            componentC(Common::make_unique<ComponentC>(root))
        {
        }

        AsyncOperationSPtr BeginFooB1(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooB1(AsyncOperationSPtr const& fooB1Operation);

        AsyncOperationSPtr BeginFooB2(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooB2(AsyncOperationSPtr const& fooB2Operation);

    private:
        unique_ptr<ComponentC> componentC;
        class FooB1AsyncOperation;
        class FooB2AsyncOperation;
    };

    class ComponentA : public ComponentRoot
    {
        DENY_COPY(ComponentA)

    public:
        ComponentA() : 
        ComponentRoot()
        {
            componentB = make_unique<ComponentB>(*this);
        }

        AsyncOperationSPtr BeginFooA(const AsyncCallback callback, AsyncOperationSPtr const& parent);
        ErrorCode EndFooA(AsyncOperationSPtr const& fooAOperation);

    private:
        unique_ptr<ComponentB> componentB;
        class FooAAsyncOperation;
    };

    // ************************************************************************************************************
    // BasicScenarioTest:
    //  This is a basic scenario test for AsyncOperation. There are three async components; ComponentA, ComponentB
    //  and ComponentC that offer an operation Foo to be performed asynchronously. A user code calls FooA operation
    //  provided on ComponentA asynchronously. Operation FooA is implemented in terms of FooB1 and FooB2 on
    //  ComponentB which in turn are implemented in terms of operations provided by ComponentC. Following is the
    // async operation hierarchy.
    //
    // FooA -> FooB1 -> FooC1.1, FooC1.2
    //      -> FooB2 -> FooC2.1, FooC2.2
    // ************************************************************************************************************
    class TestAsyncOperation
    {
    };

    static AutoResetEvent BasicScenarioTestDoneEvent(false);

    struct BasicScenarioTestHelper
    {
        static void OnFooACompleted(
            ComponentA * const componentAPtr, 
            AsyncOperationSPtr const & fooAOperation)
        {
            Trace.WriteInfo(TraceType, "OnFooACompleted callback called");
            if (!fooAOperation->CompletedSynchronously)
            {
                FinishFooA(componentAPtr, fooAOperation);
            }
        }

        static void FinishFooA(
            ComponentA * const componentAPtr, 
            AsyncOperationSPtr const & fooAOperation)
        {
            ErrorCode errorCode = componentAPtr->EndFooA(fooAOperation);
            if (CancelScenario == false)
            {
                VERIFY_IS_TRUE(errorCode.IsSuccess());
            }
            else
            {
                VERIFY_IS_TRUE(errorCode.ReadValue() == ErrorCodeValue::OperationCanceled);
                Trace.WriteInfo(TraceType, "Operation canceled successfully");
            }

            BasicScenarioTestDoneEvent.Set();
        }
    };

#pragma region Implementation of ComponentA

    class ComponentA::FooAAsyncOperation : public AsyncOperation
    {
        DENY_COPY(FooAAsyncOperation)

    public:
        FooAAsyncOperation(
            ComponentA & owner,
            const AsyncCallback callback,
            AsyncOperationSPtr const& parent)
            : owner_(owner),
            AsyncOperation(callback, parent)
        {
            Trace.WriteInfo(TraceType, "FooAAsyncOperation");
        }

        ~FooAAsyncOperation()
        {
            Trace.WriteInfo(TraceType, "~FooAAsyncOperation");
        }

        static ErrorCode End(AsyncOperationSPtr const& fooAAOperation)
        {
            auto thisPtr = AsyncOperation::End<FooAAsyncOperation>(fooAAOperation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            auto operation = owner_.componentB->BeginFooB1(
                [this](AsyncOperationSPtr const& fooB1Operation){ this->OnFooB1Completed(fooB1Operation); },
                thisSPtr);
            if (operation->CompletedSynchronously)
            {
                FinishFooB1(operation);
            }
        }

    private:
        ComponentA & owner_;

        void OnFooB1Completed(AsyncOperationSPtr const& fooB1Operation)
        {
            if (!fooB1Operation->CompletedSynchronously)
            {
                FinishFooB1(fooB1Operation);
            }
        }

        void FinishFooB1(AsyncOperationSPtr const & fooB1Operation)
        {
            ErrorCode error = owner_.componentB->EndFooB1(fooB1Operation);
            if (!error.IsSuccess())
            {
                TryComplete(fooB1Operation->Parent, error);
            }
            else
            {
                auto operation = owner_.componentB->BeginFooB2(
                    [this](AsyncOperationSPtr const& fooB2Operation){ this->OnFooB2Completed(fooB2Operation); },
                    fooB1Operation->Parent);
                if (operation->CompletedSynchronously)
                {
                    FinishFooB2(operation);
                }
            }
        }

        void OnFooB2Completed(AsyncOperationSPtr const & fooB2Operation)
        {
            if (!fooB2Operation->CompletedSynchronously)
            {
                FinishFooB2(fooB2Operation);
            }
        }

        void FinishFooB2(AsyncOperationSPtr const & fooB2Operation)
        {
            ErrorCode error = owner_.componentB->EndFooB2(fooB2Operation);
            TryComplete(fooB2Operation->Parent, error);
        }
    };

    AsyncOperationSPtr ComponentA::BeginFooA(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooA");
        return AsyncOperation::CreateAndStart<FooAAsyncOperation>(*this, callback, parent);
    }

    ErrorCode ComponentA::EndFooA(AsyncOperationSPtr const& fooAOperation)
    {
        Trace.WriteInfo(TraceType, "EndFooA");
        return FooAAsyncOperation::End(fooAOperation);
    }

#pragma endregion

#pragma region Implementation of ComponentB

    class ComponentB::FooB1AsyncOperation : public AsyncOperation
    {
        DENY_COPY(FooB1AsyncOperation)

    public:
        FooB1AsyncOperation(ComponentB & owner, const AsyncCallback callback, AsyncOperationSPtr const& parent)
            : owner_(owner),
            AsyncOperation(callback, parent)
        {
            Trace.WriteInfo(TraceType, "FooB1AsyncOperation()");
        }

        ~FooB1AsyncOperation()
        {
            Trace.WriteInfo(TraceType, "~FooB1AsyncOperation()");
        }

        static ErrorCode End(AsyncOperationSPtr const& fooB1Operation)
        {
            auto thisPtr = AsyncOperation::End<FooB1AsyncOperation>(fooB1Operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const& thisSPtr)
        {
            if (Common::CancelScenario)
            {
                Cancel();
            }
            else
            {
                auto operation = owner_.componentC->BeginFooC11(
                    [this](AsyncOperationSPtr const& fooC11Operation){ this->OnFooC11Completed(fooC11Operation); },
                    thisSPtr);
                if (operation->CompletedSynchronously)
                {
                    FinishFooC11(operation);
                }
            }
        }

        void OnCancel()
        {
            Trace.WriteInfo(TraceType, "FooB1AsyncOperation::OnCancel()");
            TryComplete(shared_from_this(), ErrorCodeValue::OperationCanceled);
        }

    private:
        ComponentB & owner_;

        void OnFooC11Completed(AsyncOperationSPtr const & fooC11Operation)
        {
            if (!fooC11Operation->CompletedSynchronously)
            {
                FinishFooC11(fooC11Operation);
            }
        }

        void FinishFooC11(AsyncOperationSPtr const & fooC11Operation)
        {
            ErrorCode error = owner_.componentC->EndFooC11(fooC11Operation);
            if (!error.IsSuccess())
            {
                TryComplete(fooC11Operation->Parent, error);
            }
            else
            {
                auto operation = owner_.componentC->BeginFooC12(
                    [this](AsyncOperationSPtr const& fooC12Operation){ this->OnFooC12Completed(fooC12Operation); },
                    fooC11Operation->Parent);
                if (operation->CompletedSynchronously)
                {
                    FinishFooC12(operation);
                }
            }
        }

        void OnFooC12Completed(AsyncOperationSPtr const& fooC12Operation)
        {
            if (!fooC12Operation->CompletedSynchronously)
            {
                FinishFooC12(fooC12Operation);
            }
        }

        void FinishFooC12(AsyncOperationSPtr const& fooC12Operation)
        {
            TryComplete(
                fooC12Operation->Parent,
                owner_.componentC->EndFooC12(fooC12Operation));
        }
    };
    class ComponentB::FooB2AsyncOperation : public AsyncOperation
    {
        DENY_COPY(FooB2AsyncOperation)

    public:
        FooB2AsyncOperation(ComponentB & owner, const AsyncCallback callback, AsyncOperationSPtr const& parent)
            : owner_(owner),
            AsyncOperation(callback, parent)
        {
            Trace.WriteInfo(TraceType, "FooB2AsyncOperation()");
        }

        ~FooB2AsyncOperation()
        {
            Trace.WriteInfo(TraceType, "~FooB2AsyncOperation()");
        }

        static ErrorCode End(AsyncOperationSPtr const& fooB2Operation)
        {
            auto thisPtr = AsyncOperation::End<FooB2AsyncOperation>(fooB2Operation);
            return thisPtr->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const& thisSPtr)
        {
            if (Common::CancelScenario)
            {
                Cancel();
            }
            else
            {
                auto operation = owner_.componentC->BeginFooC21(
                    [this](AsyncOperationSPtr const& fooC11Operation){ this->OnFooC21Completed(fooC11Operation); },
                    thisSPtr);
                if (operation->CompletedSynchronously)
                {
                    FinishFooC21(operation);
                }
            }
        }

        void OnCancel()
        {
            Trace.WriteInfo(TraceType, "FooB2AsyncOperation::OnCancel()");
            TryComplete(shared_from_this(), ErrorCodeValue::OperationCanceled);
        }

    private:
        ComponentB & owner_;

        void OnFooC21Completed(AsyncOperationSPtr const & fooC11Operation)
        {
            if (!fooC11Operation->CompletedSynchronously)
            {
                FinishFooC21(fooC11Operation);
            }
        }

        void FinishFooC21(AsyncOperationSPtr const & fooC11Operation)
        {
            ErrorCode error = owner_.componentC->EndFooC21(fooC11Operation);
            if (!error.IsSuccess())
            {
                TryComplete(fooC11Operation->Parent, error);
            }
            else
            {
                auto operation = owner_.componentC->BeginFooC22(
                    [this](AsyncOperationSPtr const& fooC12Operation){ this->OnFooC22Completed(fooC12Operation); },
                    fooC11Operation->Parent);
                if (operation->CompletedSynchronously)
                {
                    FinishFooC22(operation);
                }
            }
        }

        void OnFooC22Completed(AsyncOperationSPtr const& fooC12Operation)
        {
            if (!fooC12Operation->CompletedSynchronously)
            {
                FinishFooC22(fooC12Operation);
            }
        }

        void FinishFooC22(AsyncOperationSPtr const& fooC12Operation)
        {
            TryComplete(
                fooC12Operation->Parent,
                owner_.componentC->EndFooC22(fooC12Operation));
        }
    };

    AsyncOperationSPtr ComponentB::BeginFooB1(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooB1");
        return AsyncOperation::CreateAndStart<FooB1AsyncOperation>(*this, callback, parent);
    }

    ErrorCode ComponentB::EndFooB1(AsyncOperationSPtr const& fooB1Operation)
    {
        Trace.WriteInfo(TraceType, "EndFooB1");
        return FooB1AsyncOperation::End(fooB1Operation);
    }

    AsyncOperationSPtr ComponentB::BeginFooB2(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooB2");
        return AsyncOperation::CreateAndStart<FooB2AsyncOperation>(*this, callback, parent);
    }

    ErrorCode ComponentB::EndFooB2(AsyncOperationSPtr const& fooB2Operation)
    {
        Trace.WriteInfo(TraceType, "EndFooB2");
        return FooB2AsyncOperation::End(fooB2Operation);
    }

#pragma endregion

#pragma region Implementation of ComponentC

    AsyncOperationSPtr ComponentC::BeginFooC11(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooC11");
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    ErrorCode ComponentC::EndFooC11(AsyncOperationSPtr const& fooC11Operation)
    {
        Trace.WriteInfo(TraceType, "EndFooC11");
        return CompletedAsyncOperation::End(fooC11Operation);
    }

    AsyncOperationSPtr ComponentC::BeginFooC12(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooC12");
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    ErrorCode ComponentC::EndFooC12(AsyncOperationSPtr const& fooC12Operation)
    {
        Trace.WriteInfo(TraceType, "EndFooC12");
        return CompletedAsyncOperation::End(fooC12Operation);
    }

    AsyncOperationSPtr ComponentC::BeginFooC21(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooC21");
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    ErrorCode ComponentC::EndFooC21(AsyncOperationSPtr const& fooC21Operation)
    {
        Trace.WriteInfo(TraceType, "EndFooC21");
        return CompletedAsyncOperation::End(fooC21Operation);
    }

    AsyncOperationSPtr ComponentC::BeginFooC22(const AsyncCallback callback, AsyncOperationSPtr const& parent)
    {
        Trace.WriteInfo(TraceType, "BeginFooC22");
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }

    ErrorCode ComponentC::EndFooC22(AsyncOperationSPtr const& fooC22Operation)
    {
        Trace.WriteInfo(TraceType, "EndFooC22");
        return CompletedAsyncOperation::End(fooC22Operation);
    }

#pragma endregion

    BOOST_FIXTURE_TEST_SUITE(TestAsyncOperationSuite,TestAsyncOperation)

    BOOST_AUTO_TEST_CASE(BasicScenarioTest)
    {
        auto componentA = make_shared<ComponentA>();
        auto componentAPtr = componentA.get();

        auto operation = componentA->BeginFooA(
            [componentAPtr](AsyncOperationSPtr const& fooAOperation){ BasicScenarioTestHelper::OnFooACompleted(componentAPtr, fooAOperation); },
            componentA->CreateAsyncOperationRoot());
        if (operation->CompletedSynchronously)
        {
            BasicScenarioTestHelper::FinishFooA(componentA.get(), operation);
        }

        BasicScenarioTestDoneEvent.WaitOne(TimeSpan::MaxValue);
    }

    BOOST_AUTO_TEST_CASE(CancelScenarioTest)
    {
        Common::CancelScenario = true;  // Will cause a cancel to occur partway through

        auto componentA = make_shared<ComponentA>();
        auto componentAPtr = componentA.get();

        auto operation = componentA->BeginFooA(
            [componentAPtr](AsyncOperationSPtr const& fooAOperation){ BasicScenarioTestHelper::OnFooACompleted(componentAPtr, fooAOperation); },
            componentA->CreateAsyncOperationRoot());
        if (operation->CompletedSynchronously)
        {
            BasicScenarioTestHelper::FinishFooA(componentA.get(), operation);
        }

        BasicScenarioTestDoneEvent.WaitOne(TimeSpan::MaxValue);

        Common::CancelScenario = false;
    }

    BOOST_AUTO_TEST_CASE(CallBeginButNotEnd)
    {
        auto componentA = make_shared<ComponentA>();
        auto operation = componentA->BeginFooA(
            [componentA](AsyncOperationSPtr const &) { },
            componentA->CreateAsyncOperationRoot());

        operation->Error.ReadValue();
    }

    // ------------------------------------------------------------------------
    // Cancel Tests

    typedef std::function<void(void)> ActionVoid;

    class ScriptableOperation : public AsyncOperation
    {
    public:
        ScriptableOperation(
            AsyncCallback const & start,
            ActionVoid const cancel,
            ActionVoid const complete,
            AsyncCallback const & outerCallback,
            AsyncOperationSPtr const & parent,
            bool skipCompleteOnCancel = false)
            : AsyncOperation(outerCallback, parent, skipCompleteOnCancel),
              start_(start),
              cancel_(cancel),
              complete_(complete)
        {
        }

        __declspec(property(get=get_ChildrenCount)) size_t ChildrenCount;
        inline size_t get_ChildrenCount() const { return this->Test_ChildrenCount; }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr) { start_(thisSPtr); }
        void OnCancel() { cancel_(); }
        void OnComplete() { complete_(); }

    private:
        AsyncCallback start_;
        ActionVoid cancel_;
        ActionVoid complete_;
    };

    class SkipCompleteOnCancelOperation : public ScriptableOperation
    {
    public:
        SkipCompleteOnCancelOperation(
            AsyncCallback const & start,
            ActionVoid const cancel,
            ActionVoid const complete,
            AsyncCallback const & outerCallback,
            AsyncOperationSPtr const & parent)
            : ScriptableOperation(start, cancel, complete, outerCallback, parent, true)
        {
        }

        void Complete()
        {
            TryComplete(shared_from_this(), ErrorCodeValue::Success);
        }
    };

    BOOST_AUTO_TEST_CASE(CheckChildrenCountOnCancelledParent)
    {
        // 1. Have a parent async op
        // 2. Create a child and cancel it synchronously.
        // 3. Verify the parent's children count is 0
        std::shared_ptr<ManualResetEvent> childStartedEvent = make_shared<ManualResetEvent>();
        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};
        AsyncOperationSPtr root = AsyncOperationRoot<shared_ptr<ManualResetEvent>>::Create(childStartedEvent);

        AsyncOperationSPtr child;
        AsyncCallback childStart = [&](AsyncOperationSPtr const &) { childStartedEvent->Set(); };
        ActionVoid childCancel = [](){ };

        AsyncOperationSPtr parent;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const & thisSPtr) {
            thisSPtr->Cancel();
            child = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, []() {}, asyncCallback, thisSPtr);
        };

        parent = AsyncOperation::CreateAndStart<ScriptableOperation>(parentStart, [](){}, [](){}, asyncCallback, root);
        auto scriptableOp = static_cast<ScriptableOperation*>(parent.get());

        bool childWasStarted = childStartedEvent->WaitOne(TimeSpan::FromSeconds(10));
        ASSERT_IFNOT(childWasStarted, "Child was not started");
        ASSERT_IFNOT(scriptableOp->ChildrenCount == 0, "Children count is {0}, but expected to be 0", scriptableOp->ChildrenCount);
        parent->TryComplete(parent);
    }

    BOOST_AUTO_TEST_CASE(CheckChildrenCountOnCompletedParent)
    {
        // 1. Have a parent async op
        // 2. Create a child and cancel it synchronously.
        // 3. Verify the parent's children count is 0
        std::shared_ptr<ManualResetEvent> childStartedEvent = make_shared<ManualResetEvent>();
        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};
        AsyncOperationSPtr root = AsyncOperationRoot<shared_ptr<ManualResetEvent>>::Create(childStartedEvent);

        AsyncOperationSPtr child;
        AsyncCallback childStart = [&](AsyncOperationSPtr const &) { childStartedEvent->Set(); };
        ActionVoid childCancel = [](){ };

        AsyncOperationSPtr parent;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const & thisSPtr) {
            thisSPtr->TryComplete(thisSPtr);
            child = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, []() {}, asyncCallback, thisSPtr);
        };

        parent = AsyncOperation::CreateAndStart<ScriptableOperation>(parentStart, [](){}, [](){}, asyncCallback, root);
        auto scriptableOp = static_cast<ScriptableOperation*>(parent.get());

        bool childWasStarted = childStartedEvent->WaitOne(TimeSpan::FromSeconds(10));
        ASSERT_IFNOT(childWasStarted, "Child was not started");
        ASSERT_IFNOT(scriptableOp->ChildrenCount == 0, "Children count is {0}, but expected to be 0", scriptableOp->ChildrenCount);
        parent->TryComplete(parent);
    }

    BOOST_AUTO_TEST_CASE(EnsureChildIsSignalledDuringLongrunningOnStartWhenParentCancels)
    {
        // 1. Have a parent async op
        // 2. Create a child and have a long running event in it
        // 3. Verify the parents cancel leads to calling OnCancel() on child
        std::shared_ptr<ManualResetEvent> childCancelled = make_shared<ManualResetEvent>();
        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};
        AsyncOperationSPtr root = AsyncOperationRoot<shared_ptr<ManualResetEvent>>::Create(childCancelled);

        AsyncCallback childStart = [&](AsyncOperationSPtr const &) { bool done = childCancelled->WaitOne(TimeSpan::FromSeconds(10)); ASSERT_IFNOT(done," Child was not cancelled") };
        ActionVoid childCancel = [&]() { childCancelled->Set(); };

        AsyncOperationSPtr parent;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const &) {
        };

        parent = AsyncOperation::CreateAndStart<ScriptableOperation>(parentStart, [](){}, [](){}, asyncCallback, root);

        // Child OnStart Blocks. So create it on a threadpool
        Threadpool::Post([&]() {
            auto child = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, []() {}, asyncCallback, parent);
        });

        auto scriptableOp = static_cast<ScriptableOperation*>(parent.get());
        
        // wait for parent's child count to be 1
        int waitTime = 0;
        do
        {
            Sleep(100);
            waitTime++;
            if (waitTime == 100)
            {
                Assert::CodingError("Child not attached to parent");
            }
        } while (scriptableOp->ChildrenCount == 0);

        parent->Cancel();
 
        ASSERT_IFNOT(scriptableOp->ChildrenCount == 0, "Children count is {0}, but expected to be 0", scriptableOp->ChildrenCount);
    }

    // Verifies that child is canceled even if Cancel is called while/before child is starting
    BOOST_AUTO_TEST_CASE(CancelParentInStart)
    {
        std::shared_ptr<ManualResetEvent> childWasCanceledEvent = make_shared<ManualResetEvent>();
        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};
        AsyncOperationSPtr root = AsyncOperationRoot<shared_ptr<ManualResetEvent>>::Create(childWasCanceledEvent);

        AsyncOperationSPtr child;
        AsyncCallback childStart = [](AsyncOperationSPtr const &){};
        ActionVoid childCancel = [&](){ childWasCanceledEvent->Set(); };

        AsyncOperationSPtr parent;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const & thisSPtr) {
            thisSPtr->Cancel();
            child = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, [](){}, asyncCallback, thisSPtr);
        };

        parent = AsyncOperation::CreateAndStart<ScriptableOperation>(parentStart, [](){}, [](){}, asyncCallback, root);

        bool childWasCanceled = childWasCanceledEvent->WaitOne(TimeSpan::FromSeconds(10));
        ASSERT_IFNOT(childWasCanceled, "Child was not canceled");
        auto scriptableParent = static_cast<ScriptableOperation*>(parent.get());
        ASSERT_IFNOT(scriptableParent->ChildrenCount == 0, "No child must be attached");
    }

    BOOST_AUTO_TEST_CASE(CancelParentObserveState)
    {
        bool childWasCanceled = false;
       
        ManualResetEvent childCancelCalledEvent(false);
        ManualResetEvent childCancelCompletedEvent(false);
        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};
        AsyncOperationSPtr root = AsyncOperationRoot<bool>::Create(childWasCanceled);

        AsyncOperationSPtr child;
        AsyncCallback childStart = [](AsyncOperationSPtr const &){};
        ActionVoid childCancel = [&]()
        {
            childCancelCalledEvent.Set(); 
            ::Sleep(10000); 
            childWasCanceled = true; 
            childCancelCompletedEvent.Set();
        };

        AsyncOperationSPtr parent;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const & thisSPtr) {
            child = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, [](){}, asyncCallback, thisSPtr);
        };

        parent = AsyncOperation::CreateAndStart<ScriptableOperation>(parentStart, [](){}, [](){}, asyncCallback, root);

        Threadpool::Post([&parent] ()
        {
            parent->Cancel();
        });

        childCancelCalledEvent.WaitOne();
        ASSERT_IF(childWasCanceled, "Child should not be canceled yet");

        ASSERT_IF(parent->CompletedSynchronously, "CompletedSynchronously should not be true");

        parent->TryComplete(parent, ErrorCodeValue::Success);
        childCancelCompletedEvent.WaitOne();

        // Cancel asyncOperation with skipCompleteOnCancel=true and observe the state
         AsyncOperationSPtr skipCompleteOnCancelAsyncOp = AsyncOperation::CreateAndStart<SkipCompleteOnCancelOperation>(
                 asyncCallback, [](){}, [](){}, asyncCallback, root);

         skipCompleteOnCancelAsyncOp->Cancel();

         ASSERT_IF(skipCompleteOnCancelAsyncOp->CompletedSynchronously, "CompletedSynchronously should not be true");

         skipCompleteOnCancelAsyncOp->TryComplete(skipCompleteOnCancelAsyncOp, ErrorCodeValue::Success);
    }

    class ScriptableComOp : public ComAsyncOperationContext
    {
    public:
        ScriptableComOp(
            AsyncCallback const &start,
            ActionVoid const cancel,
            ActionVoid const complete,
            bool skipCompleteOnCancel = false)
            : ComAsyncOperationContext(skipCompleteOnCancel),
              start_(start),
              cancel_(cancel),
              complete_(complete)
        {
        }

    protected:
        void OnStart(Common::AsyncOperationSPtr const & proxySPtr)
        {
            start_(proxySPtr);
        }

        void OnCancel() { cancel_(); }
        void OnComplete() { complete_(); }

    private:
        AsyncCallback start_;
        ActionVoid cancel_;
        ActionVoid complete_;
    };

    class SkipCompleteOnCancelComOp : public ScriptableComOp
    {
    public:
        SkipCompleteOnCancelComOp(
            AsyncCallback const & start,
            ActionVoid const cancel,
            ActionVoid const complete)
            : ScriptableComOp(start, cancel, complete, true)
        {
        }
    };

    BOOST_AUTO_TEST_CASE(ComCancelAfterStart)
    {
        bool childWasCanceled = false;

        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};

        AsyncOperationSPtr childOp;
        AsyncCallback childStart = [&](AsyncOperationSPtr const &){ ASSERT_IF(childWasCanceled, "should only be canceled after child is started"); };
        ActionVoid childCancel = [&](){ childWasCanceled = true; };

        AsyncCallback parentStart = [&](AsyncOperationSPtr const & proxySPtr) {
            childOp = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, [](){}, asyncCallback, proxySPtr);
        };
        ComPointer<ScriptableComOp> parentOp = make_com<ScriptableComOp>(parentStart, [](){}, [](){});

        ComPointer<IFabricAsyncOperationContext> parentCleanup;
        ComPointer<ScriptableComOp> copyParentOp = parentOp;
        HRESULT hrParentStart = parentOp->StartAndDetach(std::move(copyParentOp), parentCleanup.InitializationAddress());

        parentCleanup->Cancel();

        ASSERT_IFNOT(SUCCEEDED(hrParentStart), "Parent StartAndDetach failed");
        ASSERT_IFNOT(childWasCanceled, "Child was not canceled");
    }

    BOOST_AUTO_TEST_CASE(ComSkipCompleteOnCancel)
    {
        bool childWasCanceled = false;

        AsyncCallback asyncCallback = [](AsyncOperationSPtr const & operation)
        {
            AsyncOperationRoot<ComAsyncOperationContextCPtr>::Get(operation->Parent)->TryComplete(operation->Parent, S_OK);
        };

        AsyncOperationSPtr childOp;
        AsyncCallback childStart = [&](AsyncOperationSPtr const &){ ASSERT_IF(childWasCanceled, "should only be canceled after child is started"); };
        ActionVoid childCancel = [&]()
        { 
            ASSERT_IF(childWasCanceled, "Child cancel is  already invoked");
            childWasCanceled = true; 
        };

        AsyncCallback parentStart = [&](AsyncOperationSPtr const & proxySPtr) {
            childOp = AsyncOperation::CreateAndStart<SkipCompleteOnCancelOperation>(
                childStart, childCancel, [](){}, asyncCallback, proxySPtr);
        };
        ComPointer<SkipCompleteOnCancelComOp> parentOp = make_com<SkipCompleteOnCancelComOp>(parentStart, [](){}, [](){});

        ComPointer<IFabricAsyncOperationContext> parentCleanup;
        ComPointer<SkipCompleteOnCancelComOp> copyParentOp = parentOp;
        HRESULT hrParentStart = parentOp->StartAndDetach(std::move(copyParentOp), parentCleanup.InitializationAddress());

        parentCleanup->Cancel();
        
        ASSERT_IFNOT(SUCCEEDED(hrParentStart), "Parent StartAndDetach failed");
        ASSERT_IFNOT(childWasCanceled, "Child was not canceled");
        ASSERT_IF(parentCleanup->IsCompleted() == TRUE, "Parent should not complete on cancel");

        // call second time to esnure oncancel is called once
        parentCleanup->Cancel();

        // Now complete the child
        SkipCompleteOnCancelOperation* skipCompleteOnCancelOp = static_cast<SkipCompleteOnCancelOperation *>(childOp.get());
        skipCompleteOnCancelOp->Complete();

        ASSERT_IFNOT(parentCleanup->IsCompleted() == TRUE, "Parent should be completed");
    }

    BOOST_AUTO_TEST_CASE(ComCancelParentInStart)
    {
        bool childWasCanceled = false;

        AsyncCallback asyncCallback = [](AsyncOperationSPtr const &){};

        AsyncOperationSPtr childOp;
        AsyncCallback childStart = [&](AsyncOperationSPtr const &){ ASSERT_IF(childWasCanceled, "should only be canceled after child is started"); };
        ActionVoid childCancel = [&]()
        { 
            ASSERT_IF(childWasCanceled, "Child cancel is  already invoked");
            childWasCanceled = true; 
        };

        ComPointer<ScriptableComOp> parentOp;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const & proxySPtr) {
            parentOp->Cancel();
            // Call second time to ensure OnCancel is called once
            parentOp->Cancel();
            childOp = AsyncOperation::CreateAndStart<ScriptableOperation>(
                childStart, childCancel, [](){}, asyncCallback, proxySPtr);
        };
        parentOp = make_com<ScriptableComOp>(parentStart, [](){}, [](){});

        ComPointer<IFabricAsyncOperationContext> parentCleanup;
        ComPointer<ScriptableComOp> copyParentOp = parentOp;
        HRESULT hrParentStart = parentOp->StartAndDetach(std::move(copyParentOp), parentCleanup.InitializationAddress());

        ASSERT_IFNOT(SUCCEEDED(hrParentStart), "Parent StartAndDetach failed");
        ASSERT_IFNOT(childWasCanceled, "Child was not canceled");
    }

    BOOST_AUTO_TEST_CASE(ComSkipCompleteOnCancelInStart)
    {
        bool childWasCanceled = false;

        AsyncCallback asyncCallback = [](AsyncOperationSPtr const & operation)
        {
            AsyncOperationRoot<ComAsyncOperationContextCPtr>::Get(operation->Parent)->TryComplete(operation->Parent, S_OK);
        };

        AsyncOperationSPtr childOp;
        AsyncCallback childStart = [&](AsyncOperationSPtr const &){ ASSERT_IF(childWasCanceled, "should only be canceled after child is started"); };
        ActionVoid childCancel = [&](){ childWasCanceled = true; };

        ComPointer<SkipCompleteOnCancelComOp> parentOp;
        AsyncCallback parentStart = [&](AsyncOperationSPtr const & proxySPtr) {
            parentOp->Cancel();
            childOp = AsyncOperation::CreateAndStart<SkipCompleteOnCancelOperation>(
                childStart, childCancel, [](){}, asyncCallback, proxySPtr);
        };
        parentOp = make_com<SkipCompleteOnCancelComOp>(parentStart, [](){}, [](){});

        ComPointer<IFabricAsyncOperationContext> parentCleanup;
        ComPointer<SkipCompleteOnCancelComOp> copyParentOp = parentOp;
        HRESULT hrParentStart = parentOp->StartAndDetach(std::move(copyParentOp), parentCleanup.InitializationAddress());

        ASSERT_IFNOT(SUCCEEDED(hrParentStart), "Parent StartAndDetach failed");
        ASSERT_IFNOT(childWasCanceled, "Child should be canceled");
        ASSERT_IF(parentCleanup->IsCompleted() == TRUE, "Parent should not complete on cancel");

        // Now complete the child
        SkipCompleteOnCancelOperation* skipCompleteOnCancelOp = static_cast<SkipCompleteOnCancelOperation *>(childOp.get());
        skipCompleteOnCancelOp->Complete();

        ASSERT_IFNOT(parentCleanup->IsCompleted() == TRUE, "Parent should be completed");
    }

    BOOST_AUTO_TEST_SUITE_END()
}
