// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ImageBuilderAsyncOperationExecutor
        {
            DENY_COPY(ImageBuilderAsyncOperationExecutor)

        public:
            explicit ImageBuilderAsyncOperationExecutor(
                __in ProcessRolloutContextAsyncOperationBase & owner);

            ~ImageBuilderAsyncOperationExecutor();

            void AddOperation(Common::AsyncOperationSPtr const & asyncOperation);
            void ExternalCancel();
            void ResetOperation();

            void WaitProgressCallback();
            bool OnUpdateProgressDetails(std::wstring const & details);
            void OnUpdateProgressDetailsComplete();

        private:
            // Must be called under reader or writer lock.
            bool IsImageBuilderAsyncOperationPending_UnderLock() const;

        private:
            ProcessRolloutContextAsyncOperationBase & owner_;
            Common::AsyncOperationSPtr imageBuilderAsyncOperation_;
            Common::RwLock imageBuilderAsyncOperationLock_;
            std::shared_ptr<Common::ManualResetEvent> progressCallbackEvent_;
            bool canceledCalled_;
        };

        using ImageBuilderAsyncOperationExecutorUPtr = std::unique_ptr<ImageBuilderAsyncOperationExecutor>;
    }
}
