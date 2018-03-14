// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostRegistration :
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationHostRegistration)

    public:
        ApplicationHostRegistration(std::wstring const & hostId, ApplicationHostType::Enum hostType);
        ~ApplicationHostRegistration();

        __declspec(property(get=get_Id)) std::wstring const & HostId;
        inline std::wstring const & get_Id() const { return hostId_; };

        __declspec(property(get=get_Type)) ApplicationHostType::Enum HostType;
        inline ApplicationHostType::Enum get_Type() const { return hostType_; };

        ApplicationHostRegistrationStatus::Enum GetStatus() const;
        
        Common::ErrorCode OnStartRegistration(Common::TimerSPtr && timer);
        Common::ErrorCode OnMonitoringInitialized(Common::ProcessWaitSPtr && processMonitor);
        Common::ErrorCode OnFinishRegistration();
        Common::ErrorCode OnRegistrationTimedout();

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        mutable Common::RwLock lock_;
        std::wstring hostId_;
        ApplicationHostType::Enum hostType_;
        ApplicationHostRegistrationStatus::Enum status_;
        Common::ProcessWaitSPtr hostProcessMonitor_;
        Common::TimerSPtr finishRegistrationTimer_;
    };
}
