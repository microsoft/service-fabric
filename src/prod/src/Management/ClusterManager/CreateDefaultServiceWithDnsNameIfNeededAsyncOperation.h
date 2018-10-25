// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This async operation encapsulates default service creation with DNS name (if needed) logic.
// This will be used by both create and upgrade application workflows.
// Both ensure that they(Owner) remain valid for the lifetime of this async operation. 
// Passed Service context also remains valid for the lifetime of async operation as it is owned by the owner. 
// For more details refer following functions: 
//   a. ProcessApplicationUpgradeContextAsyncOperation::CreateDefaultServices
//   b. ProcessApplicationContextAsyncOperation::CreateServices

namespace Management
{
    namespace ClusterManager
    {
        class CreateDefaultServiceWithDnsNameIfNeededAsyncOperation : public TimedAsyncOperation
        {
            DENY_COPY(CreateDefaultServiceWithDnsNameIfNeededAsyncOperation);

        public:
            CreateDefaultServiceWithDnsNameIfNeededAsyncOperation(
                ServiceContext const &defaultServiceContext,
                __in ProcessRolloutContextAsyncOperationBase &owner,
                Common::ActivityId const &activityId,
                TimeSpan timeout,
                AsyncCallback callback,
                AsyncOperationSPtr const &parent);

            static ErrorCode End(AsyncOperationSPtr const &operation);

        protected:
            void OnStart(AsyncOperationSPtr const &thisSPtr);

        private:
            void OnCreateDnsNameComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);
            void OnCreateServiceComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        private:
            const ServiceContext &defaultServiceContext_;
            ProcessRolloutContextAsyncOperationBase &owner_;
            const Common::ActivityId activityId_;
        };
    }
}

