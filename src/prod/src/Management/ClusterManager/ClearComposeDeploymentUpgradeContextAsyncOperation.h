// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClearComposeDeploymentUpgradeContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ClearComposeDeploymentUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in ComposeDeploymentUpgradeContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

        private:
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            ComposeDeploymentUpgradeContext & context_;
        };
    }
}


