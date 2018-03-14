// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        /*
            Base class for all async op that invoke user API

            The End for each derived class must get the error code from the EndHelper

            The EndHelper is responsible for creating the correct ProxyErrorCode object
            based on the value returned by the TryStartUserApi method

            If the user API was not invoked then the proxy error code will have an invalid user api 
        */
        class FailoverUnitProxy::UserApiInvokerAsyncOperationBase : public Common::AsyncOperation
        {
            DENY_COPY(UserApiInvokerAsyncOperationBase);

        public:
            UserApiInvokerAsyncOperationBase(
                FailoverUnitProxy & owner,
                Common::ApiMonitoring::InterfaceName::Enum interfaceName,
                Common::ApiMonitoring::ApiName::Enum api, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) :
                AsyncOperation(callback, parent),
                owner_(owner),
                invoker_(owner, interfaceName, api),
                interface_(interfaceName)
            {
            }

            virtual ~UserApiInvokerAsyncOperationBase() {}

        protected:
            ProxyErrorCode EndHelper();

            void TraceBeforeStart(
                Common::AcquireExclusiveLock & proxyLock) const;

            // Tries to start the API
            // If the API cannot be started this async op is completed with RAProxyIncompatibleOperationWithCurrentState
            bool TryStartUserApi(
                Common::AcquireExclusiveLock & proxyLock,
                Common::AsyncOperationSPtr const & thisSPtr);

            bool TryStartUserApi(
                Common::AcquireExclusiveLock & proxyLock,
                std::wstring const & metadata,
                Common::AsyncOperationSPtr const & thisSPtr);

            // Called after the begin has been invoked
            // Checks if the operation should be cancelled or not and stores the ptr to the async op
            void TryContinueUserApi(
                Common::AsyncOperationSPtr const & userOperation);

            // Called after the user api has finished
            void FinishUserApi(
                Common::AcquireWriteLock & proxyLock,
                Common::ErrorCode const & errorCode);

            FailoverUnitProxy & owner_;

            // This field is required by the Open and ChangeRole ops to trace out the endpoint
            // Ideally this should be removed
            Common::ApiMonitoring::InterfaceName::Enum const interface_;

        private:
            UserApiInvoker invoker_;
        };
    }
}
