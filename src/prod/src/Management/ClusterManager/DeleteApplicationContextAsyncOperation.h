// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteApplicationContextAsyncOperation :
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            DeleteApplicationContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            DeleteApplicationContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationContext &,
                __in bool keepContextAlive,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:
            void OnCommitDeleteApplicationComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        private:
            Common::ErrorCode ReadServiceContexts(__inout std::vector<ServiceContext> & serviceContexts);
            
            void DeleteAllServices(Common::AsyncOperationSPtr const &);

            void ParallelDeleteServices(Common::AsyncOperationSPtr const &);
            void OnParallelDeleteServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::ErrorCode ScheduleDeleteService(Common::AsyncOperationSPtr const &, size_t operationIndex, ParallelOperationsCompletedCallback const &);
            Common::AsyncOperationSPtr BeginDeleteService(Common::AsyncOperationSPtr const &, size_t operationIndex, Common::AsyncCallback const &);
            void EndDeleteService(Common::AsyncOperationSPtr const &, size_t, ParallelOperationsCompletedCallback const &);

            void OnDeleteServiceDnsNameComplete(Common::AsyncOperationSPtr const &, ParallelOperationsCompletedCallback const &);

            void DeleteApplicationFromFM(Common::AsyncOperationSPtr const &);
            void OnDeleteApplicationFromFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            Common::AsyncOperationSPtr BeginDeleteApplicationProperty(Common::AsyncOperationSPtr const &, Common::AsyncCallback const &);
            void EndDeleteApplicationProperty(Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginDeleteApplicationName(Common::AsyncOperationSPtr const &, Common::AsyncCallback const &);
            void EndDeleteApplicationName(Common::AsyncOperationSPtr const &);

            void CleanupImageStore(Common::AsyncOperationSPtr const &);
            void OnCleanupImageStoreComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void DeleteApplicationHealth(Common::AsyncOperationSPtr const &);
            void OnDeleteApplicationHealthComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void PrepareCommitDeleteApplication(Common::AsyncOperationSPtr const &);
            virtual void CommitDeleteApplication(Common::AsyncOperationSPtr const &, Store::StoreTransaction &);
            void OnCompleted() override;

            ApplicationContext & context_;
            Common::NamingUri appName_;

            // service names may be a super-set of service contexts if there were pending create services
            // at the time of application create failure
            //
            std::vector<ServiceContext> serviceContexts_;
            std::vector<ServiceModelServiceNameEx> serviceNames_;

            // Optimization for replacing scenario. ApplicationContext will not be deleted if set to true.
            bool keepContextAlive_;
        };
    }
}
