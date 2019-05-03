// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct cgroup;

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

        Common::ErrorCode Cleanup(IProcessActivationContextSPtr const & activationContext, bool processClosedUsingCtrlC, UINT uExitCode);

        //In case the caller does not need the pCgroupObj null should be passed
        //Else it is the callers responsibility to free the cgroup structure with cgroup_free
        //if forSpOnly is true cgroupName should represent the sp cgroup path
        static int CreateCgroup(std::string const & cgroupName, const ServiceModel::ResourceGovernancePolicyDescription * resourceGovernanceDescription, 
          const ServiceModel::ServicePackageResourceGovernanceDescription * servicePackageResourceDescription, struct cgroup ** pCgroupObj, bool forSpOnly = false);

        //This needs to be called in case a process/container activation fails and we already called CreateCgroup
        //Also call this when the process/container terminates
        //if forSpOnly is true cgroupName should represent the sp cgroup path
        static int RemoveCgroup(std::string const & cgroupName, bool forSpOnly = false);

        Common::ErrorCode UpdateRGPolicy(
            IProcessActivationContextSPtr & activationContext,
            ProcessDescriptionUPtr & processDescription);

        //Used for fetching info about cgroup limits - only for tests
        static int Test_QueryCgroupInfo(std::string const & cgroupName, uint64 & cpuLimit, uint64 & memoryLimit);

        //Used for getting usage from cgroup
        static int GetCgroupUsage(std::string const & cgroupName, uint64 & cpuUsage, uint64 & memoryUsage);

        //Use this to synchronize access to cgroup hierarchy as the libcgroup library is not really threadsafe
        static Common::RwLock cgroupLock_;

        static UINT ProcessDeactivateExitCode;
        static UINT ProcessAbortExitCode;

    private:
        class ActivateAsyncOperation;
        friend class ActivateAsyncOperation;

        class DeactivateAsyncOperation;
        friend class DeactivateAsyncOperation;

    private:
        std::wstring fabricBinFolder_;
    };
}
