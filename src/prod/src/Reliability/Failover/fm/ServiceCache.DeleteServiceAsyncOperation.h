// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::DeleteServiceAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(DeleteServiceAsyncOperation);

        public:

            DeleteServiceAsyncOperation(
                ServiceCache & serviceCache,
                std::wstring const& serviceName,
                bool const isForce,
                uint64 serviceInstance,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~DeleteServiceAsyncOperation();

            __declspec (property(get = get_ServiceName)) std::wstring const& ServiceName;
            std::wstring const& get_ServiceName() const { return serviceName_; }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            void OnTombstoneServiceCreated(Common::AsyncOperationSPtr const& createOperation);

            void StartDeleteService(Common::AsyncOperationSPtr const& thisSPtr);

            void OnStoreUpdateCompleted(Common::AsyncOperationSPtr const& updateOperation);

            ServiceCache & serviceCache_;

            std::wstring serviceName_;
            bool isForce_;
            uint64 serviceInstance_;

            ServiceInfoSPtr newServiceInfo_;
            LockedServiceInfo lockedServiceInfo_;
        };
    }
}
