// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::RepartitionServiceAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(RepartitionServiceAsyncOperation)

        public:

            RepartitionServiceAsyncOperation(
                ServiceCache & serviceCache,
                LockedServiceInfo && lockedServiceInfo,
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && added,
                std::vector<ConsistencyUnitDescription> && removed,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~RepartitionServiceAsyncOperation();

            static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            void AddPartitions(Common::AsyncOperationSPtr const& thisSPtr);
            void RemovePartitions(Common::AsyncOperationSPtr const& thisSPtr);

            void OnTombstoneServiceCreated(Common::AsyncOperationSPtr const& createOperation);
            void OnStoreUpdateCompleted(Common::AsyncOperationSPtr const& updateOperation);

            ServiceCache & serviceCache_;

            LockedServiceInfo lockedServiceInfo_;

            ServiceDescription serviceDescription_;
            std::map<FailoverUnitId, ConsistencyUnitDescription> added_;
            std::map<FailoverUnitId, ConsistencyUnitDescription> removed_;

            ServiceInfoSPtr newServiceInfo_;
            std::vector<FailoverUnitUPtr> failoverUnits_;
        };
    }
}
