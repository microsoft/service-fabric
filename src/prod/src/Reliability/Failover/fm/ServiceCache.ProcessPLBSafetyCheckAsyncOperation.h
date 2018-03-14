// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceCache::ProcessPLBSafetyCheckAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(ProcessPLBSafetyCheckAsyncOperation);

        public:

            ProcessPLBSafetyCheckAsyncOperation(
                ServiceCache & serviceCache,
                ServiceModel::ApplicationIdentifier && appId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            ~ProcessPLBSafetyCheckAsyncOperation();

            __declspec (property(get = get_ApplicationId)) ServiceModel::ApplicationIdentifier const& ApplicationId;
            ServiceModel::ApplicationIdentifier const& get_ApplicationId() const { return appId; }

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const& operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            void OnApplicationUpdateCompleted(Common::AsyncOperationSPtr const& updateOperation);

            ServiceCache & serviceCache_;

            ServiceModel::ApplicationIdentifier appId;
        };
    }
}
