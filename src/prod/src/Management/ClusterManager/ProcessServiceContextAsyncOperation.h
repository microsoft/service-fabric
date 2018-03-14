// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessServiceContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessServiceContextAsyncOperation(
                __in RolloutManager &,
                __in ServiceContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        private:
            Common::AsyncOperationSPtr BeginCreateService(Common::AsyncOperationSPtr const &, Common::AsyncCallback const &);
            void EndCreateService(Common::AsyncOperationSPtr const &);

            void FinishCreateService(Common::AsyncOperationSPtr const &);
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        private:
            void OnCreateDnsNameComplete(AsyncOperationSPtr const &, bool);
            Common::AsyncOperationSPtr BeginCheckServiceDnsName(Common::AsyncOperationSPtr const &, Common::ByteBuffer &, Common::AsyncCallback const &);
            void EndCheckServiceDnsName(Common::AsyncOperationSPtr const &);

        private:
            ServiceContext & context_;
            Common::NamingUri dnsUri_;
        };
    }
}
