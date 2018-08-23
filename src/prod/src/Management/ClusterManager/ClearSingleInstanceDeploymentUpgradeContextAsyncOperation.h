// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClearSingleInstanceDeploymentUpgradeContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ClearSingleInstanceDeploymentUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in SingleInstanceDeploymentUpgradeContext &,
                Common::TimeSpan const &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &) override;

        private:
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            SingleInstanceDeploymentUpgradeContext & context_;
        };
    }
}
