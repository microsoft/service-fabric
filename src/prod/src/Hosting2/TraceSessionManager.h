// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class TraceSessionManager
        : public Common::RootedObject
        , public Common::FabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(TraceSessionManager)

    public:
        TraceSessionManager(Common::ComponentRoot const & root);

        virtual ~TraceSessionManager();

    protected:
        virtual Common::ErrorCode OnOpen();

        virtual Common::ErrorCode OnClose();

        virtual void OnAbort();

    private:
        void OnTimer();
        Common::ErrorCode TraceSessionManager::Shutdown();

    private:
        Common::TimerSPtr timer_;
        Common::TimeSpan pollingInterval_;
        Common::ExclusiveLock processingLock_;
    };
}
