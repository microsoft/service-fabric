// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IInfrastructureService
    {
    public:
        virtual ~IInfrastructureService() {};

        virtual Common::AsyncOperationSPtr BeginRunCommand(
            bool isAdminCommand,
            std::wstring const & command,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndRunCommand(
            Common::AsyncOperationSPtr const & asyncOperation,
            std::wstring & reply) = 0;

        virtual Common::AsyncOperationSPtr BeginReportStartTaskSuccess(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndReportStartTaskSuccess(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginReportFinishTaskSuccess(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndReportFinishTaskSuccess(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginReportTaskFailure(
            std::wstring const & taskId,
            uint64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndReportTaskFailure(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;
    };
}
