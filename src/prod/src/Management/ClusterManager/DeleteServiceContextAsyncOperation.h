// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteServiceContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            DeleteServiceContextAsyncOperation(
                __in RolloutManager &,
                __in ServiceContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            Common::AsyncOperationSPtr BeginDeleteService(Common::AsyncOperationSPtr const &, Common::AsyncCallback const &);
            void EndDeleteService(Common::AsyncOperationSPtr const &);
            void FinishDelete(Common::AsyncOperationSPtr const &);
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnDeleteServiceDnsNameComplete(Common::AsyncOperationSPtr const &);

            ServiceContext & context_;
        };
    }
}
