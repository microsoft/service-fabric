// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ProcessActivationContext : public IProcessActivationContext,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ProcessActivationContext)

    public:
        ProcessActivationContext(DWORD processId, std::wstring cgroupName = L"");

        static void CreateProcessActivationContext(DWORD processId, std::wstring cgroupName, ProcessActivationContextSPtr& activationContext);

        ~ProcessActivationContext();

        __declspec(property(get=get_ProcessMonotor)) Common::ProcessWaitSPtr const & ProcessMonitor;
        inline Common::ProcessWaitSPtr const & get_ProcessMonotor() const { return processMonitor_; };

        __declspec(property(get = get_DebugProcessMonotor)) Common::ProcessWaitSPtr const & DebugProcessMonitor;
        inline Common::ProcessWaitSPtr const & get_DebugProcessMonotor() const { return debugprocessMonitor_; };

        __declspec(property(get=get_ProcessHandle)) Common::ProcessHandleUPtr const & ProcessHandle;
        inline Common::ProcessHandleUPtr const & get_ProcessHandle() const { return processHandle_; };

        __declspec(property(get=get_JobHandle)) Common::JobHandleUPtr const & JobHandle;
        inline Common::JobHandleUPtr const & get_JobHandle() const { return jobHandle_; };

        __declspec(property(get=get_ThreadHandle)) Common::HandleUPtr const & ThreadHandle;
        inline Common::HandleUPtr const & get_ThreadHandle() const { return threadHandle_; };

        __declspec(property(get=get_WorkstationHandle)) Common::WorkstationHandleUPtr const & WorkstationHandleObj;
        inline Common::WorkstationHandleUPtr const & get_WorkstationHandle() const { return workstationHandle_; };

        __declspec(property(get=get_DesktopHandle)) Common::DesktopHandleUPtr const & DesktopHandleObj;
        inline Common::DesktopHandleUPtr const & get_DesktopHandle() const { return desktopHandle_; };

        __declspec(property(get=get_ProcessId)) DWORD ProcessId;
        inline DWORD get_ProcessId() const { return processId_; };

        __declspec(property(get = get_ContainerId)) wstring ContainerId;
        inline wstring get_ContainerId() const { return containerId_; };

        __declspec(property(get = get_DebuggerProcessId)) DWORD DebuggerProcessId;
        inline DWORD get_DebuggerProcessId() const { return debuggerProcId_; };

        __declspec(property(get = get_DebuggerThreadHandle)) Common::HandleUPtr const & DebuggerThreadHandle;
        inline Common::HandleUPtr const & get_DebuggerThreadHandle() const { return debuggerThreadHandle_; };

        __declspec(property(get=get_ProcessStdoutRedirector)) ProcessConsoleRedirectorSPtr const & ProcessStdoutRedirector;
        inline ProcessConsoleRedirectorSPtr const & get_ProcessStdoutRedirector() const { return processStdoutRedirector_; }

        __declspec(property(get=get_ProcessStderrRedirector)) ProcessConsoleRedirectorSPtr const & ProcessStderrRedirector;
        inline ProcessConsoleRedirectorSPtr const & get_ProcessStderrRedirector() const { return processStderrRedirector_; }

        __declspec(property(get=get_HasConsole)) bool const & HasConsole;
        inline bool const & get_HasConsole() const { return hasConsole_; }

        __declspec(property(get=get_CgroupName)) std::wstring const & CgroupName;
        inline std::wstring const & get_CgroupName() const { return cgroupName_; }

        virtual Common::ErrorCode TerminateAndCleanup(bool processGracefulExited, UINT uExitCode);
        virtual Common::ErrorCode ResumeProcess();

    private:
        Common::ProcessWaitSPtr const processMonitor_;
        Common::ProcessWaitSPtr const debugprocessMonitor_;
        Common::ProcessHandleUPtr const processHandle_;
        Common::JobHandleUPtr const jobHandle_;
        Common::HandleUPtr const threadHandle_;
        Common::WorkstationHandleUPtr const workstationHandle_;
        Common::DesktopHandleUPtr const desktopHandle_;
        wstring containerId_;
        DWORD processId_;
        DWORD debuggerProcId_;
        Common::ProcessHandleUPtr const debuggerProcessHandle_;
        Common::HandleUPtr const debuggerThreadHandle_;
        ProcessConsoleRedirectorSPtr processStdoutRedirector_;
        ProcessConsoleRedirectorSPtr processStderrRedirector_;
        bool hasConsole_;
        std::wstring cgroupName_;
    };
}
