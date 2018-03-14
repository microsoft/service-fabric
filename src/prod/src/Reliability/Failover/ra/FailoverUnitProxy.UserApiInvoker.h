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
            All user APIs are invoked via this class
        */
        class FailoverUnitProxy::UserApiInvoker
        {
            DENY_COPY(UserApiInvoker);

        public:
            UserApiInvoker(
                FailoverUnitProxy & owner,
                Common::ApiMonitoring::InterfaceName::Enum interfaceName,
                Common::ApiMonitoring::ApiName::Enum api) :
                owner_(owner),
                interface_(interfaceName),
                api_(api)
            {
                ASSERT_IF(interface_ != Common::ApiMonitoring::InterfaceName::IReplicator &&
                    interface_ != Common::ApiMonitoring::InterfaceName::IStatefulServiceReplica &&
                    interface_ != Common::ApiMonitoring::InterfaceName::IStatelessServiceInstance,
                    "Unknown interface value {0}", interface_);
            }

            __declspec(property(get = get_Interface)) Common::ApiMonitoring::InterfaceName::Enum Interface;
            Common::ApiMonitoring::InterfaceName::Enum get_Interface() const { return interface_; }

            __declspec(property(get = get_Api)) Common::ApiMonitoring::ApiName::Enum Api;
            Common::ApiMonitoring::ApiName::Enum get_Api() const { return api_; }

            ProxyErrorCode GetError(Common::ErrorCode && userApiError);

            void TraceBeforeStart(
                Common::AcquireExclusiveLock & proxyLock) const;

            void TraceBeforeStart(
                Common::AcquireExclusiveLock & proxyLock,
                Common::TraceCorrelatedEventBase const & data) const;

            // Tries to start a sync API
            bool TryStartUserApi(
                Common::AcquireExclusiveLock & proxyLock);

            bool TryStartUserApi(
                Common::AcquireExclusiveLock & proxyLock,
                std::wstring const & metadata);

            // Called after the user api has finished
            void FinishUserApi(
                Common::AcquireWriteLock & proxyLock,
                Common::ErrorCode const & errorCode);

            FailoverUnitProxyOperationManagerBase & GetOperationManager(
                Common::AcquireWriteLock & proxyLock);

        private:
            FailoverUnitProxy & owner_;
            Common::ApiMonitoring::InterfaceName::Enum interface_;
            Common::ApiMonitoring::ApiName::Enum api_;
            Common::ApiMonitoring::ApiNameDescription description_;
        };
    }
}



