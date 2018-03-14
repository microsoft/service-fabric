// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyInfrastructureService :
        public Common::ComponentRoot,
        public IInfrastructureService
    {
        DENY_COPY(ComProxyInfrastructureService);

    public:
        ComProxyInfrastructureService(Common::ComPointer<IFabricInfrastructureService> const & comImpl);
        virtual ~ComProxyInfrastructureService();

        virtual Common::AsyncOperationSPtr BeginRunCommand(
            bool isAdminCommand,
            std::wstring const & command,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndRunCommand(
            Common::AsyncOperationSPtr const & asyncOperation,
            std::wstring & reply);

        virtual Common::AsyncOperationSPtr BeginReportStartTaskSuccess(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndReportStartTaskSuccess(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginReportFinishTaskSuccess(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndReportFinishTaskSuccess(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginReportTaskFailure(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndReportTaskFailure(
            Common::AsyncOperationSPtr const & asyncOperation);

    private:
        class RunCommandAsyncOperation;
        class ReportTaskAsyncOperationBase;
        class ReportStartTaskSuccessAsyncOperation;
        class ReportFinishTaskSuccessAsyncOperation;
        class ReportTaskFailureAsyncOperation;

        Common::ComPointer<IFabricInfrastructureService> const comImpl_;
    };
}
