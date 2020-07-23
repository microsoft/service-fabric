// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IProcessActivationContext
    {
        

    public:
        IProcessActivationContext() 
        {
        }

        virtual ~IProcessActivationContext()
        {
        }

        virtual Common::ErrorCode TerminateAndCleanup(bool processGracefulExited, UINT uExitCode) = 0;
        virtual Common::ErrorCode ResumeProcess() = 0;

        __declspec(property(get=get_ProcessId)) DWORD ProcessId;
        virtual DWORD get_ProcessId() const = 0;

        __declspec(property(get = get_ContainerId)) wstring ContainerId;
        virtual wstring get_ContainerId() const = 0;

        __declspec(property(get = get_DebuggerProcessId)) DWORD DebuggerProcessId;
        virtual DWORD get_DebuggerProcessId() const = 0;

        __declspec(property(get=get_HasConsole)) bool const & HasConsole;
        virtual bool const & get_HasConsole() const = 0;
    };
}
