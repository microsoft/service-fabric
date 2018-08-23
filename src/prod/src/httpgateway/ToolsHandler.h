// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class ToolsHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ToolsHandler)

    public:

        ToolsHandler(HttpGatewayImpl & server);
        virtual ~ToolsHandler();

        Common::ErrorCode Initialize();

    private:
        void ToolsHandler::StartChaos(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnStartChaosComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;

        void ToolsHandler::StopChaos(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnStopChaosComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;

        void ToolsHandler::GetChaos(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnGetChaosComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;

        void ToolsHandler::GetChaosReport(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnGetChaosReportComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;

        void ToolsHandler::GetChaosEvents(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnGetChaosEventsComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;

        void ToolsHandler::GetChaosSchedule(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnGetChaosScheduleComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;

        void ToolsHandler::PostChaosSchedule(__in Common::AsyncOperationSPtr const& thisSPtr);
        void ToolsHandler::OnPostChaosScheduleComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously) const;
    };
}

