// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostTerminatedEventArgs : public Common::EventArgs
    {
    public:
        ApplicationHostTerminatedEventArgs (
            Common::ActivityDescription const & activityDescription,
            std::wstring const & hostId, 
            DWORD exitCode)
            : activityDescription_(activityDescription)
            , hostId_(hostId)
            , exitCode_(exitCode)
        {
        }
        
        __declspec(property(get=get_ActivityDescription)) Common::ActivityDescription const & ActivityDescription;
        inline Common::ActivityDescription const & get_ActivityDescription() const { return activityDescription_; }

        __declspec(property(get=get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostId_; }

        __declspec(property(get=get_ExitCode)) DWORD ExitCode;
        inline DWORD get_ExitCode() const { return exitCode_; }

    private:
        Common::ActivityDescription activityDescription_;
        std::wstring hostId_;
        DWORD exitCode_;
    };
}
