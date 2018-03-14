// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessApplicationContextUpdateAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessApplicationContextUpdateAsyncOperation(
                __in RolloutManager &,
                __in ApplicationUpdateContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &,
                bool forwardOperation = true);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            // Same async operation is used for updating application capacities and for reverting them.
            // In case of update this variable is set to true.
            // In case of failure (and we are reverting), this variable is set to false.
            bool forwardOperation_;

            void OnRequestToFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void CommitContexts(Common::AsyncOperationSPtr const &);
            void DeleteUpdateContext(Common::AsyncOperationSPtr const &);
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            ApplicationUpdateContext & context_;
            ApplicationContext applicationContext_;
            ServiceModel::ApplicationIdentifier appId_;
        };
    }
}

