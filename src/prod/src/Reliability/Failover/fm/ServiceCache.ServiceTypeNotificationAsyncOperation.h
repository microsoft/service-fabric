// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::ServiceTypeNotificationAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(ServiceTypeNotificationAsyncOperation);

        public:

            ServiceTypeNotificationAsyncOperation(
                ServiceCache & serviceCache,
                std::vector<ServiceTypeInfo> && incomingInfos,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~ServiceTypeNotificationAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const& operation,
                __out std::vector<ServiceTypeInfo> & processedInfos);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            void OnServiceTypeNotificationCompleted(
                Common::AsyncOperationSPtr const& operation,
				ServiceTypeInfo & serviceTypeInfo,
				LockedServiceType & lockedServiceType,
                ServiceTypeSPtr & newServiceType);

            ServiceCache & serviceCache_;

            std::vector<ServiceTypeInfo> incomingInfos_;
            std::vector<ServiceTypeInfo> processedInfos_;

            Common::atomic_uint64 pendingCount_;

            MUTABLE_RWLOCK(FM.ServiceCache.ServiceTypeNotificationAsyncOperation, lock_);
        };
    }
}
