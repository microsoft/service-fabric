// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessApplicationTypeContextAsyncOperation : 
            public ProcessProvisionContextAsyncOperationBase
        {
            DENY_COPY(ProcessApplicationTypeContextAsyncOperation)

        public:
            ProcessApplicationTypeContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationTypeContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void ExternalCancel();
            void OnCompleted() override;

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            void StartProvisioning(Common::AsyncOperationSPtr const &) override;
            void RefreshContext(Store::StoreTransaction const &);
            void OnBuildApplicationTypeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void FinishBuildApplicationType(Common::AsyncOperationSPtr const &);

            void OnUpdateProgressDetails(Common::AsyncOperationSPtr const &, std::wstring const &);
            void OnCommitProgressDetailsComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        private:
            std::wstring applicationManifestId_;
            ImageBuilderAsyncOperationExecutorUPtr imageBuilderAsyncOperationExecutor_;
        };

        using ProcessApplicationTypeContextAsyncOperationSPtr = std::shared_ptr<ProcessApplicationTypeContextAsyncOperation>;
    }
}
