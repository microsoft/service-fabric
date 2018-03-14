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
            std::wstring const & hostId, 
            DWORD exitCode)
            : hostId_(hostId)
            , exitCode_(exitCode)
        {
        }

        __declspec(property(get=get_HostId)) std::wstring const & HostId;
        inline std::wstring const & get_HostId() const { return hostId_; }

        __declspec(property(get=get_ExitCode)) DWORD ExitCode;
        inline DWORD get_ExitCode() const { return exitCode_; }

    private:
        std::wstring hostId_;
        DWORD exitCode_;
    };
}
