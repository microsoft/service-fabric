// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::ProcessUpgradeAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(ProcessUpgradeAsyncOperation);

        public:

            ProcessUpgradeAsyncOperation(
                ServiceCache & serviceCache,
                UpgradeApplicationRequestMessageBody && requestBody,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~ProcessUpgradeAsyncOperation();

            __declspec (property(get = get_ApplicationId)) ServiceModel::ApplicationIdentifier const& ApplicationId;
            ServiceModel::ApplicationIdentifier const& get_ApplicationId() const { return applicationId_; }

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const& operation,
                __out UpgradeApplicationReplyMessageBody & replyBody);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            void OnApplicationUpdateCompleted(Common::AsyncOperationSPtr const& updateOperation, bool isNewUpgrade);

            ServiceCache & serviceCache_;

            ServiceModel::ApplicationIdentifier applicationId_;
            uint64 instanceId_;
            
            UpgradeApplicationRequestMessageBody requestBody_;
            UpgradeApplicationReplyMessageBody replyBody_;
        };
    }
}
