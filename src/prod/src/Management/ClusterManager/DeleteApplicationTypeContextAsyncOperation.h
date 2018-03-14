// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteApplicationTypeContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            DeleteApplicationTypeContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationTypeContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnCompleted() override;
            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        private:
            Common::ErrorCode CheckCleanup_IgnoreCase(
                Store::StoreTransaction const & storeTx,
                __out bool & shouldCleanup,
                __out bool & isLastApplicationType);
            virtual void CommitDeleteApplicationType(Common::AsyncOperationSPtr const &, Store::StoreTransaction &tx);

            ApplicationTypeContext & context_;
            bool startedUnprovision_;
        };
    }
}
