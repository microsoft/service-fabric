// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessFabricProvisionContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessFabricProvisionContextAsyncOperation(
                __in RolloutManager &,
                __in FabricProvisionContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            FabricProvisionContext & context_;
        };
    }
}
