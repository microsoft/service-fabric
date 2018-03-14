// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::UpdateServiceTypesAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(UpdateServiceTypesAsyncOperation);

        public:

            UpdateServiceTypesAsyncOperation(
                ServiceCache & serviceCache,
                std::vector<ServiceModel::ServiceTypeIdentifier> && serviceTypeIds,
                uint64 sequenceNumber,
                Federation::NodeId const& nodeId,
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~UpdateServiceTypesAsyncOperation();

            static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

        private:

            void OnUpdateServiceTypeCompleted(
                Common::AsyncOperationSPtr const& operation,
                Common::ErrorCode error);

            ServiceCache & serviceCache_;

            std::vector<ServiceModel::ServiceTypeIdentifier> serviceTypeIds_;
            uint64 sequenceNumber_;
            Federation::NodeId nodeId_;
            ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent_;

            Common::atomic_uint64 pendingCount_;
            Common::FirstErrorTracker errorTracker_;
        };
    }
}
