// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessApplicationContextAsyncOperation : 
            public ProcessRolloutContextAsyncOperationBase
        {
        public:
            ProcessApplicationContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationContext &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual void OnStart(Common::AsyncOperationSPtr const &);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:
            void OnCommitPendingDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            ApplicationContext & context_;

            DigestedApplicationDescription appDescription_;
            std::vector<ServiceContext> defaultServices_;

            void SendCreateApplicationRequestToFM(Common::AsyncOperationSPtr const &);
            virtual void CreateServices(Common::AsyncOperationSPtr const &);
            void OnCreateServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::ErrorCode ScheduleCreateService(Common::AsyncOperationSPtr const &, size_t operationIndex, ParallelOperationsCompletedCallback const &);

        private:

            //
            // This asyncoperation serves as the base async operation for 
            // 1. Regular CreateApplication workflow 
            // 2. Implicit create application during docker compose based application creates.
            // #1 is a synchronous operation from the client whereas #2 is an async operation that
            // happens in the background. So the operation timeouts for #1 and #2 would be different.
            //
            virtual Common::TimeSpan GetCommunicationTimeout();
            virtual Common::TimeSpan GetImageBuilderTimeout();

            void OnRequestToFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::AsyncOperationSPtr BeginCreateName(Common::AsyncOperationSPtr const &, Common::AsyncCallback const &);
            void EndCreateName(Common::AsyncOperationSPtr const &);
            
            Common::AsyncOperationSPtr BeginCreateProperty(Common::AsyncOperationSPtr const &, Common::AsyncCallback const &);
            virtual void EndCreateProperty(Common::AsyncOperationSPtr const &);

            void OnBuildApplicationComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            virtual void CheckDefaultServicePackages(AsyncOperationSPtr const & thisSPtr, ErrorCode && error);

            Common::AsyncOperationSPtr BeginCreateService(Common::AsyncOperationSPtr const &, size_t operationIndex, Common::AsyncCallback const &);
            void EndCreateService(Common::AsyncOperationSPtr const &, ParallelOperationsCompletedCallback const &);

            virtual void FinishCreateApplication(Common::AsyncOperationSPtr const &);
            void OnReportApplicationPolicyComplete(__in ApplicationContext &, Common::AsyncOperationSPtr const &, Common::ErrorCode const &);
            void OnCompleted() override;
        };
    }
}
