// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IInternalInfrastructureServiceClient
    {
    public:
        virtual ~IInternalInfrastructureServiceClient() {};

        virtual Common::AsyncOperationSPtr BeginRunCommand(
            std::wstring const &command,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::ErrorCode EndRunCommand(
            Common::AsyncOperationSPtr const &,
            std::wstring &) = 0;

        virtual Common::AsyncOperationSPtr BeginReportStartTaskSuccess(
            std::wstring const &taskId,
            ULONGLONG instanceId,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::ErrorCode EndReportStartTaskSuccess(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginReportFinishTaskSuccess(
            std::wstring const &taskId,
            ULONGLONG instanceId,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::ErrorCode EndReportFinishTaskSuccess(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginReportTaskFailure(
            std::wstring const &taskId,
            ULONGLONG instanceId,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::ErrorCode EndReportTaskFailure(Common::AsyncOperationSPtr const &) = 0;
    };
}
