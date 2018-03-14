// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::CreateServiceAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(CreateServiceAsyncOperation);

        public:

            CreateServiceAsyncOperation(
                ServiceCache & serviceCache,
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
                bool isCreateFromRebuild,
                bool isServiceUpdateNeeded,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~CreateServiceAsyncOperation();

            static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

            void FinishCreateService(Common::AsyncOperationSPtr const& thisSPtr, Common::ErrorCode error, int64 commitDuration, int64 plbDuration);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            void OnStoreUpdateCompleted(Common::AsyncOperationSPtr const& updateOperation);

            void Revert();

            std::vector<FailoverUnitUPtr> CreateFailoverUnitsFromService(ServiceInfoSPtr & service);

            ServiceCache & serviceCache_;

            ServiceDescription serviceDescription_;
            std::vector<ConsistencyUnitDescription> consistencyUnitDescriptions_;
            bool isCreateFromRebuild_;
            bool isServiceUpdateNeeded_;

            ServiceInfoSPtr service_;

            LockedApplicationInfo lockedApplication_;
            LockedServiceType lockedServiceType_;
            LockedServiceInfo lockedService_;
            std::vector<FailoverUnitUPtr> failoverUnits_;

            bool isNewApplication_;
            bool isNewServiceType_;
            bool isNewService_;

            FABRIC_SEQUENCE_NUMBER healthSequence_;
        };
    }
}
