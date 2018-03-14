// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/WaitHandle.h"
#include "CompletedAsyncOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;

    class FabricComponentTestAsyncOperation : public AsyncOperation
    {
        DENY_COPY(FabricComponentTestAsyncOperation)

    public:
        FabricComponentTestAsyncOperation(
            ErrorCode error,
            TimeSpan timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & asyncOperationParent)
            : error_(error),
            timeoutHelper_(timeout),
            AsyncOperation(callback, asyncOperationParent)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const& operation)
        {
            auto thisPtr = AsyncOperation::End<FabricComponentTestAsyncOperation>(operation);
            return thisPtr->Error;
        }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            Threadpool::Post([this, thisSPtr]()
            {
                OnTimeout(thisSPtr);
            }, timeoutHelper_.GetRemainingTime());
        }

        virtual void OnTimeout(AsyncOperationSPtr const & operation)
        {
            TryComplete(operation, error_);
        }

    private:

        ErrorCode error_;
        TimeoutHelper timeoutHelper_;
    };

    class FauxAsyncFabricComponent : public AsyncFabricComponent
    {
    public:
        FauxAsyncFabricComponent()
            : error_(ErrorCodeValue::Success)
        {
            error_.ReadValue();
        }

        __declspec(property(put=set_Error)) ErrorCode Error;
        void set_Error(ErrorCode error)
        {
            error_ = error;
        }

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent)
        {
            return AsyncOperation::CreateAndStart<FabricComponentTestAsyncOperation>(error_, timeout, callback, parent);
        }

        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation)
        {
            return FabricComponentTestAsyncOperation::End(asyncOperation);
        }

        Common::AsyncOperationSPtr OnBeginClose(
            TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent)
        {
            return AsyncOperation::CreateAndStart<FabricComponentTestAsyncOperation>(error_, timeout, callback, parent);
        }

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation)
        {
            auto errorCode = FabricComponentTestAsyncOperation::End(asyncOperation);
            return errorCode;
        }

        void OnAbort()
        {
            error_.ReadValue();
        }

        ErrorCode error_;
    };

    class AsyncFabricComponentTest
    {
    };


    BOOST_FIXTURE_TEST_SUITE(AsyncFabricComponentTestSuite,AsyncFabricComponentTest)

    BOOST_AUTO_TEST_CASE(AsyncFabricComponentMemoryTest)
    {
        ManualResetEvent stopper(false);
        FauxAsyncFabricComponent component;
        component.BeginOpen(
            TimeSpan::Zero,
            [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component.EndOpen(operation).IsSuccess());
            stopper.Set();
        });
        stopper.WaitOne();
        stopper.Reset();
        component.BeginClose(
            TimeSpan::Zero,
            [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
        {
            VERIFY_IS_TRUE(component.EndClose(operation).IsSuccess());
            stopper.Set();
        });
        stopper.WaitOne();
    }

    BOOST_AUTO_TEST_CASE(FabricComponentStateTest)
    {
        ManualResetEvent stopper(false);
        FauxAsyncFabricComponent component;
        TimeSpan openCloseTimeout = TimeSpan::FromSeconds(5);

        //Assert constructed component is in Created state
        VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Created);

        component.BeginOpen(
            openCloseTimeout,
            [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component.EndOpen(operation).IsSuccess());
            VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opened);
            stopper.Set();
        });

        VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opening);
        stopper.WaitOne();

        stopper.Reset();
        component.BeginClose(
            openCloseTimeout,
            [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
        {
            VERIFY_IS_TRUE(component.EndClose(operation).IsSuccess());
            VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closed);
            stopper.Set();
        });

        VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closing);
        stopper.WaitOne();

        component.Abort();
        VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Aborted);
    }

    BOOST_AUTO_TEST_CASE(FabricComponentTransitionToAbortedTest)
    {
        ManualResetEvent stopper(false);
        //Assert Created->Aborted is valid
        FauxAsyncFabricComponent component1;
        VERIFY_IS_TRUE(component1.State.Value == FabricComponentState::Created);
        component1.Abort();
        VERIFY_IS_TRUE(component1.State.Value == FabricComponentState::Aborted);

        //Assert Opened->Aborted is valid
        FauxAsyncFabricComponent component2;
        TimeSpan openCloseTimeout = TimeSpan::Zero;
        component2.BeginOpen(
            openCloseTimeout,
            [&component2, &stopper] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component2.EndOpen(operation).IsSuccess());
            VERIFY_IS_TRUE(component2.State.Value == FabricComponentState::Opened);
            stopper.Set();
        });

        stopper.WaitOne();
        component2.Abort();
        VERIFY_IS_TRUE(component2.State.Value == FabricComponentState::Aborted);
        stopper.Reset();

        //Assert Opening->Aborted is valid and component stays in Opening state at 
        //end of Abort and moves into Aborted once open completes
        FauxAsyncFabricComponent component3;
        openCloseTimeout = TimeSpan::FromSeconds(5);
        component3.BeginOpen(
            openCloseTimeout,
            [&component3, &stopper] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component3.EndOpen(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
            VERIFY_IS_TRUE(component3.State.Value == FabricComponentState::Aborted);
            stopper.Set();
        });

        VERIFY_IS_TRUE(component3.State.Value == FabricComponentState::Opening);
        component3.Abort();
        VERIFY_IS_TRUE(component3.State.Value == FabricComponentState::Opening);
        stopper.WaitOne();

        stopper.Reset();

        //Assert Closed->Aborted is valid. Already verified in FabricComponentStateTest

        //Assert Closing->Aborted is valid and component stays in Closing state at 
        //end of Abort and moves into Aborted once close completes
        FauxAsyncFabricComponent component4;
        openCloseTimeout = TimeSpan::Zero;
        component4.BeginOpen(
            openCloseTimeout,
            [&component4, &stopper] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component4.EndOpen(operation).IsSuccess());
            VERIFY_IS_TRUE(component4.State.Value == FabricComponentState::Opened);
            stopper.Set();
        });

        stopper.WaitOne();
        stopper.Reset();

        openCloseTimeout = TimeSpan::FromSeconds(5);
        component4.BeginClose(
            openCloseTimeout,
            [&component4, &stopper] (AsyncOperationSPtr const& operation) -> void 
        {
            VERIFY_IS_TRUE(component4.EndClose(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
            VERIFY_IS_TRUE(component4.State.Value == FabricComponentState::Aborted);
            stopper.Set();
        });

        VERIFY_IS_TRUE(component4.State.Value == FabricComponentState::Closing);
        component4.Abort();
        VERIFY_IS_TRUE(component4.State.Value == FabricComponentState::Closing);
        stopper.WaitOne();

        //Assert Aborted->Aborted is valid
        component4.Abort();
        VERIFY_IS_TRUE(component4.State.Value == FabricComponentState::Aborted);
    }

    
    BOOST_AUTO_TEST_CASE(FabricComponentFailOpenCloseTest)
    {
        ManualResetEvent stopper(false);
        FauxAsyncFabricComponent component1;
        ErrorCode error = ErrorCode(ErrorCodeValue::UnspecifiedError);
        component1.Error = error;

        TimeSpan openCloseTimeout = TimeSpan::Zero;
        component1.BeginOpen(
            openCloseTimeout,
            [&component1, &stopper, &error] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component1.EndOpen(operation).ReadValue() == error.ReadValue());
            VERIFY_IS_TRUE(component1.State.Value == FabricComponentState::Aborted);
            stopper.Set();
        });

        stopper.WaitOne();
        stopper.Reset();

        FauxAsyncFabricComponent component2;
        component2.BeginOpen(
            openCloseTimeout,
            [&component2, &stopper] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component2.EndOpen(operation).IsSuccess());
            VERIFY_IS_TRUE(component2.State.Value == FabricComponentState::Opened);
            stopper.Set();
        });

        stopper.WaitOne();
        stopper.Reset();

        component2.Error = error;
        component2.BeginClose(
            openCloseTimeout,
            [&component2, &stopper, &error] (AsyncOperationSPtr const& operation) -> void 
        { 
            VERIFY_IS_TRUE(component2.EndClose(operation).ReadValue() == error.ReadValue());
            VERIFY_IS_TRUE(component2.State.Value == FabricComponentState::Aborted);
            stopper.Set();
        });

        stopper.WaitOne();
    }

    BOOST_AUTO_TEST_CASE(FabricComponentInvalidTransitionTest)
    {
        ManualResetEvent stopper(false);
        TimeSpan openCloseTimeout = TimeSpan::Zero;
        {
            //Assert Created->Closing is invalid
            FauxAsyncFabricComponent component;
            VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Created);
            component.BeginClose(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndClose(operation).ReadValue() == ErrorCodeValue::InvalidState);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Created);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            //Assert Opening->Closing, Opening->Opening is invalid & at end of Opening->Closing the component is aborted
            openCloseTimeout = TimeSpan::FromSeconds(5);
            component.BeginOpen(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndOpen(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Aborted);
                stopper.Set();
            });

            VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opening);
            component.BeginClose(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndClose(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opening);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            component.BeginOpen(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndOpen(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opening);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            stopper.WaitOne();
            VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Aborted);
        }

        {
            stopper.Reset();
            FauxAsyncFabricComponent component;
            openCloseTimeout = TimeSpan::Zero;
            component.BeginOpen(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndOpen(operation).IsSuccess());
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opened);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            //Assert Opened->Opening is invalid
            component.BeginOpen(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndOpen(operation).ReadValue() == ErrorCodeValue::InvalidState);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Opened);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            //Assert Closing->Closing, Closing->Opening is invalid
            openCloseTimeout = TimeSpan::FromSeconds(5);
            component.BeginClose(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndClose(operation).IsSuccess());
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closed);
                stopper.Set();
            });

            VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closing);
            component.BeginOpen(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndOpen(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closing);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            component.BeginClose(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndClose(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closing);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            stopper.WaitOne();

            //Assert Closed->Closing, Closed->Opening is invalid
            component.BeginOpen(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndOpen(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closed);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();

            component.BeginClose(
                openCloseTimeout,
                [&component, &stopper] (AsyncOperationSPtr const& operation) -> void 
            { 
                VERIFY_IS_TRUE(component.EndClose(operation).ReadValue() == ErrorCodeValue::FabricComponentAborted);
                VERIFY_IS_TRUE(component.State.Value == FabricComponentState::Closed);
                stopper.Set();
            });

            stopper.WaitOne();
            stopper.Reset();
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
