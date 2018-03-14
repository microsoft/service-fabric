// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ProcessActivator :
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ProcessActivator)

    public:
        ProcessActivator(
            Common::ComponentRoot const & root);
        virtual ~ProcessActivator();

        Common::AsyncOperationSPtr BeginActivate(
            SecurityUserSPtr const & runAs,
            ProcessDescription const & processDescription,
            std::wstring const & fabricbinPath,
            Common::ProcessWait::WaitCallback const & processExitCallback,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            bool allowWinfabAdminSync = false);

        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation,
            __out IProcessActivationContextSPtr & activationContext);

        Common::AsyncOperationSPtr BeginDeactivate(
            IProcessActivationContextSPtr const & activationContext,            
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode Terminate(IProcessActivationContextSPtr const & activationContext, UINT uExitCode);

        Common::ErrorCode Test_QueryJobObject(IProcessActivationContext const & activationContext, uint64 & cpuRate, uint64 & memoryLimit) const;

        static UINT ProcessDeactivateExitCode;
        static UINT ProcessAbortExitCode;

    private:
        Common::ErrorCode EnsureSeDebugPrivilege();
        Common::ErrorCode EnsureAssignPrimaryTokenPrivilege();
        Common::ErrorCode IsSystemAccount(bool & isSystem);

        Common::ErrorCode Cleanup(IProcessActivationContextSPtr const & activationContext, bool terminateProcess, UINT uExitCode);

    private:
        class ActivateAsyncOperation;
        friend class ActivateAsyncOperation;

        class DeactivateAsyncOperation;
        friend class DeactivateAsyncOperation;

    private:
        std::wstring fabricBinFolder_;
        Common::RwLock seDebugPrivilegeCheckLock_;
        bool hasSeDebugPrivilege_;
        Common::RwLock assignPrimaryTokenPrivilegeCheckLock_;
        bool hasAssignPrimaryTokenPrivilege_;
        Common::RwLock systemAccountCheckLock_;
        bool isSystem_;
        bool accountChecked_;
        Common::ExclusiveLock desktopRestricationLock_;
        Common::ExclusiveLock logonAsUserLock_;
        Common::ExclusiveLock consoleRedirectionLock_;
    };
}
