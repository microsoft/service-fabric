//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        // This async op deletes the existing single instance deployment and then change its context status to Pending for another creation.
        // It doesn't delete ApplicationContext and SingleInstanceDeploymentContext for sake of performance. Instead, update their fields and status.
        // Before completing, this async op also does the preparation steps for creating a single instance deployment.
        // Note that there is no rollback of a replacement as the entities have been deleted. The context would fail and furthermore to be deleted even if this async op returns failure.
        class ReplaceSingleInstanceDeploymentContextAsyncOperation :
            public DeleteSingleInstanceDeploymentContextAsyncOperation
        {
        public:
            ReplaceSingleInstanceDeploymentContextAsyncOperation(
                __in RolloutManager &,
                __in SingleInstanceDeploymentContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

        private:
            virtual void InnerDeleteApplication(Common::AsyncOperationSPtr const &) override;
            virtual void CompleteDeletion(Common::AsyncOperationSPtr const &) override;
            void OnFinishDeleteComplete(Common::AsyncOperationSPtr const & operation, bool) override;
            void OnCommitSwitchReplaceToCreateComplete(Common::AsyncOperationSPtr const &, bool);
        };
    }
}
