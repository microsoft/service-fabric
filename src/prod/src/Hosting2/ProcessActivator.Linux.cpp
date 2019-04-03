// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "sys/prctl.h"
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <semaphore.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType_Activator("ProcessActivator");

// The HRESULT values of the fabric error codes are of 0x8007XXXX format. The win32 error values of these
// error codes can be got by dropping the 0x8007 part of the HRESULT.
UINT ProcessActivator::ProcessDeactivateExitCode = ErrorCodeValue::ProcessDeactivated & 0x0000FFFF;
UINT ProcessActivator::ProcessAbortExitCode = ErrorCodeValue::ProcessAborted & 0x0000FFFF;

class ProcessActivatorThreadSingleton
{
    DENY_COPY(ProcessActivatorThreadSingleton)

private:
    class ProcessStartupInfoItem
    {
        friend class ProcessActivatorThreadSingleton;

    private:
        char* filename_;
        char** argv_;
        char** envp_;
        char* workdir_;
        uid_t uid_;
        bool detach_;
        ProcessWait::WaitCallback callback_;

        sem_t sema_;
        ErrorCode error_;
        bool processed_;
        pid_t processId_;

        const ResourceGovernancePolicyDescription * resourceGovernanceDescription_;
        const ServicePackageResourceGovernanceDescription * servicePackageResourceDescription_;
        std::string cgroupName_;

    public:
        ProcessStartupInfoItem(char* filename, char** argv, char** envp, char* workdir, uid_t uid, bool detach, ProcessWait::WaitCallback const & callback, const ResourceGovernancePolicyDescription * resourceGovernanceDescription, const ServicePackageResourceGovernanceDescription * servicePackageResourceDescription, const std::string & cgroupName) :
            filename_(filename), argv_(argv), envp_(envp), workdir_(workdir), uid_(uid), detach_(detach), callback_(callback), resourceGovernanceDescription_(resourceGovernanceDescription), servicePackageResourceDescription_(servicePackageResourceDescription), cgroupName_(cgroupName)
        {
            processed_ = false;
            processId_ = 0;
            sem_init(&sema_, 0, 0);
            error_ = ErrorCodeValue::Success;
        }

        bool IsProcessed()   { return processed_; }

        void SetProcessed()  { processed_ = true; }

        pid_t GetProcessId() { return processId_; }

        void WaitForCompletion()
        {
            int ret = 0;
            do
            {
                ret = sem_wait(&sema_);
            } while (ret != 0 && errno == EINTR);
        }

        void SetCompleted() { sem_post(&sema_); }

        ErrorCode GetError() { return error_; }
    };

public:
    static ProcessActivatorThreadSingleton* GetSingleton()
    {
        static ProcessActivatorThreadSingleton* pSingleton = Create();
        return pSingleton;
    }

    ErrorCode CreateProcess(char* filename, char** argv, char** envp, char* workdir, uid_t uid, bool detach,
      ProcessWait::WaitCallback const & callback, pid_t &processid, const ResourceGovernancePolicyDescription * rg,  const ServicePackageResourceGovernanceDescription * spRg, const std::string cgroupName)
    {
        ProcessStartupInfoItem * item = new ProcessStartupInfoItem(filename, argv, envp, workdir, uid, detach, callback, rg, spRg, cgroupName);

        // push into queue
        pthread_mutex_lock(&queueLock_);
        queue_.push_back(item);
        pthread_mutex_unlock(&queueLock_);

        // ask the Thread to process
        sem_post(&queueSema_);

        // wait till process activation is finished
        item->WaitForCompletion();
        ErrorCode error = item->GetError();
        processid = item->GetProcessId();

        pthread_mutex_lock(&queueLock_);
        queue_.remove(item);
        pthread_mutex_unlock(&queueLock_);
        delete item;

        return error;
    }

private:
    sem_t queueSema_;
    pthread_mutex_t queueLock_;
    list<ProcessStartupInfoItem *> queue_;

    ProcessActivatorThreadSingleton() 
    {
        pthread_mutex_init(&queueLock_, NULL);
        sem_init(&queueSema_, 0, 0);

        // start the Thread
        int err;
        pthread_t pthread;
        pthread_attr_t attr;

        err = pthread_attr_init(&attr);
        _ASSERTE(err == 0);

        err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        _ASSERTE(err == 0);

        err = pthread_create(&pthread, &attr, ThreadStart, this);
        _ASSERTE(err == 0);

        err = pthread_attr_destroy(&attr);
        _ASSERTE(err == 0);
    }

    static ProcessActivatorThreadSingleton* Create()
    {
        ProcessActivatorThreadSingleton* pSingleton = new ProcessActivatorThreadSingleton();

        return pSingleton;
    }

    static void* ThreadStart(void* arg)
    {
        ProcessActivatorThreadSingleton *pObj = (ProcessActivatorThreadSingleton*)arg;
        ProcessWaitSPtr pwait;

        while(true) {
            int ret = 0;
            do {
                ret = sem_wait(&pObj->queueSema_);
            } while (ret != 0 && errno == EINTR);

            pthread_mutex_lock(&pObj->queueLock_);
            ProcessStartupInfoItem *item = 0;
            for (auto i = pObj->queue_.begin(); i != pObj->queue_.end(); ++i)
            {
                if (!(*i)->IsProcessed())
                {
                    item = (*i);  break;
                }
            }
            pthread_mutex_unlock(&pObj->queueLock_);

            if (item)
            {
                // get gid in parent to avoid deadlock
                long ngroups_max;
                ngroups_max = sysconf(_SC_NGROUPS_MAX);
                struct passwd pwd;
                struct passwd *pw;
                int bufsz = sysconf(_SC_GETPW_R_SIZE_MAX);
                bufsz = bufsz > 0 ? bufsz : 1024;
                char* buf = new char[bufsz];
                gid_t *groups = new gid_t[ngroups_max];
                int ngroups = ngroups_max;
                pid_t pID, ppid;
                ProcessWait::WaitCallback exitCallback = item->callback_;
                struct cgroup *cgroupObj = nullptr;

                int ret = getpwuid_r(item->uid_, &pwd, buf, bufsz, &pw);
                if ( 0 != ret || pw == 0)
                {
                    TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator,
                                 "ProcessActivator failed to get uid info for {0}. return: {1}, errno: {2}",
                                 item->uid_, ret, errno);
                    item->error_.Overwrite(ErrorCodeValue::OperationFailed);
                    goto Cleanup;
                }

                ret = getgrouplist(pw->pw_name, pw->pw_gid, (gid_t *) groups, &ngroups);
                if (ret == -1)
                {
                    TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator,
                                 "ProcessActivator failed to get groups for {0}, return: {1}, errno: {2}",
                                 item->uid_, ret, errno);
                    item->error_.Overwrite(ErrorCodeValue::OperationFailed);
                    goto Cleanup;
                }

                ProcessWait::Setup(); // Must be called before first process creation

                if (!item->cgroupName_.empty())
                {
                    auto error = ProcessActivator::CreateCgroup(item->cgroupName_, item->resourceGovernanceDescription_, item->servicePackageResourceDescription_, &cgroupObj);
                    if (error)
                    {
                        TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator,
                                     "Cgroup setup failed for {0} with error {1}, description of error if available {2}",
                                     item->cgroupName_, error, cgroup_get_last_errno());
                        item->error_.Overwrite(ErrorCodeValue::OperationFailed);
                        goto Cleanup;
                    }
                    else
                    {
                        TraceInfo(TraceTaskCodes::Hosting, TraceType_Activator,
                                     "Cgroup setup success for {0}",
                                     item->cgroupName_);
                    }
                }

                ppid = getpid();
                pID = fork();
                if (pID == 0)  // child
                {
                    //add the process to the appropriate cgroup
                    if (cgroupObj)
                    {
                        auto error = cgroup_attach_task_pid(cgroupObj, getpid());
                        if (error)
                        {
                            TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator,
                                 "Cgroup process attach failed for {0} with error {1}, description of error if available {2}",
                                 item->cgroupName_, error, cgroup_get_last_errno());
                            abort();
                        }
                    }
                    if(item->workdir_)
                    {
                        DIR* dir = opendir(item->workdir_);
                        if(dir)
                        {
                            closedir(dir);
                            chdir(item->workdir_);
                        }
                    }

                    // make it session leader
                    setsid();

                    // setup pseudo tty
                    int fdm, fds;
                    char* pts_name;
                    fdm = posix_openpt(O_RDWR);
                    if (fdm < 0 || unlockpt(fdm) < 0
                        || (pts_name = ptsname(fdm)) == 0
                        || (fds = open(pts_name, O_RDWR)) == 0
                        || ioctl(fds, TIOCSCTTY, NULL) < 0)
                    {
                        TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator, "ProcessActivator: setup ptty failed with error {0}", errno);
                        abort();
                    }

                    if (dup2(fds, STDIN_FILENO) < 0)
                    {
                        TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator, "ProcessActivator: dup stdio failed with error {0}", errno);
                        abort();
                    }

                    // set gid and groups
                    if (setgid(pw->pw_gid) == -1)
                    {
                        TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator, "ProcessActivator: setgid failed with error {0}", errno);
                        abort();
                    }
                    if (0 == getuid() || 0 == geteuid())
                    {
                        if (setgroups(ngroups, groups) == -1)
                        {
                            TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator, "ProcessActivator: setgroups failed with error {0}", errno);
                            abort();
                        }
                    }

                    // setuid
                    if(item->uid_ && setuid(item->uid_) != 0)
                    {
                        TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator, "ProcessActivator: setuid failed with error {0}", errno);
                        abort();
                    }

                    if (!item->detach_)
                    {
                        // if parent exits, signal SIGKILL
                        prctl(PR_SET_PDEATHSIG, SIGKILL);
                        if (ppid != getppid()) abort();
                    }

                    // umask
                    //umask(0002);

                    // execute
                    auto retv=execve(item->filename_, item->argv_, item->envp_);
                    TraceInfo(TraceTaskCodes::Hosting, TraceType_Activator, "ProcessActivator: execve failed to start {0}. ret={1}, err={2}", item->filename_, retv, errno);

                    // in case execve fails
                    abort();
                }

                // parent
                if (pID < 0)  // failed to fork
                {
                    item->error_.Overwrite(ErrorCodeValue::OperationFailed);
                    TraceWarning(TraceTaskCodes::Hosting, TraceType_Activator, "fork failed. err={0}.", errno);
                    goto Cleanup;
                }

                item->error_.Overwrite(ErrorCodeValue::Success);
                item->processId_ = pID;

                pwait = ProcessWait::Create();
                pwait->StartWait(
                    pID,
                    [pwait, exitCallback](pid_t pid, ErrorCode const & waitResult, DWORD exitCode)
                    {
                        TraceInfo(TraceTaskCodes::Hosting, TraceType_Activator, "wait callback called for process {0}, waitResult={1}, exitCode={2}", pid, waitResult, exitCode);
                        DWORD signalCode = WTERMSIG(exitCode);
                        if (signalCode == SIGINT)
                        {
                            exitCode = STATUS_CONTROL_C_EXIT;
                        }
                        else if (signalCode == SIGKILL)
                        {
                            exitCode = ProcessActivator::ProcessDeactivateExitCode;
                        }
                        exitCallback(pid, waitResult, exitCode);
                    });

Cleanup:
                delete[] buf;
                delete[] groups;
                if (cgroupObj)
                {
                    cgroup_free(&cgroupObj);
                }

                // no matter success or not, it has been processed.
                item->processed_ = true;
                sem_post(&item->sema_);
            }
        }
    }
};

// ********************************************************************************************************************
// ProcessActivator::ActivateAsyncOperation Implementation
//
class ProcessActivator::ActivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in ProcessActivator & owner,
        SecurityUserSPtr const & runAs,
        ProcessDescription const & processDescription,
        wstring const & fabricbinPath,
        ProcessWait::WaitCallback const & processExitCallback,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        runAs_(runAs),
        processDescription_(processDescription),
        processExitCallback_(processExitCallback),
        fabricBinFolder_(fabricbinPath)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, 
        __out IProcessActivationContextSPtr & activationContext)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            ProcessActivationContextSPtr context;

            ProcessActivationContext::CreateProcessActivationContext(thisPtr->processId_, thisPtr->processDescription_.CgroupName, context);

            activationContext = move(context);
        }
        //it might happen that something fails after the cgroup setup
        //therefore we clean it up here
        else if (!thisPtr->processDescription_.CgroupName.empty())
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(thisPtr->processDescription_.CgroupName, cgroupName);
            int error = ProcessActivator::RemoveCgroup(cgroupName);
            WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                      TraceType_Activator,
                      thisPtr->owner_.Root.TraceId,
                      "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                      thisPtr->processDescription_.CgroupName, error, cgroup_get_last_errno());
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CreateNonSandboxedProcess(thisSPtr);
    }

private:
    void CreateNonSandboxedProcess(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Creating NonSandboxedProcess. ProcessDescription={0}",
            this->processDescription_);

        // retrieve arguments
        string arguments;
        StringUtility::Utf16ToUtf8(this->processDescription_.Arguments, arguments);
        istringstream buf(arguments);
        copy(istream_iterator<string>(buf),
             istream_iterator<string>(),
             back_inserter(this->arguments_));

        // retrieve environment variables
        vector<wchar_t> envBlock = processDescription_.EnvironmentBlock;
        LPVOID penvBlock = envBlock.data();
        Environment::FromEnvironmentBlock(penvBlock, envMap_);

        auto error = DecryptEnvironmentVariables();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        // create process
        error = this->CreateProcess();
        if (!error.IsSuccess()) 
        {
            TryComplete(thisSPtr, error); 
            return; 
        }

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

    ErrorCode DecryptEnvironmentVariables()
    {
        ErrorCode error(ErrorCodeValue::Success);
        wstring value;
        for (auto const& setting : processDescription_.EncryptedEnvironmentVariables)
        {
            error = ContainerConfigHelper::DecryptValue(setting.second, value);
            if (!error.IsSuccess())
            {
                return error;
            }

            envMap_[setting.first] = value;
        }

        return error;
    }

    ErrorCode CreateProcess()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Launching process via fork and execve");

        char** env = nullptr;
        if (!envMap_.empty())
        {
            const char* libpathNameA = "LD_LIBRARY_PATH";
            const wstring libpathNameW = L"LD_LIBRARY_PATH";
            const char* hostEnvLibpathValueA = getenv(libpathNameA);
            if (hostEnvLibpathValueA)
            {
                wstring& envMapLibpathValueW = envMap_[libpathNameW];
                wstring hostEnvLibpathValueW;
                StringUtility::Utf8ToUtf16(hostEnvLibpathValueA, hostEnvLibpathValueW);
                if (envMapLibpathValueW.empty())
                {
                    envMapLibpathValueW = hostEnvLibpathValueW;
                }
                // Quick check to avoid adding host's lib path environment again
                // for fabric processes (FabricGateway.exe, FileStoreService.exe etc.)
                // which may already have it specified in their environment map.
                // Note this quick check just looks at the entire value of the variable
                // and not individual subpaths.
                else if (envMapLibpathValueW.compare(hostEnvLibpathValueW) != 0)
                {
                    envMapLibpathValueW += L":" + hostEnvLibpathValueW;
                }
            }

            int envIndex = 0;
            env = new char *[envMap_.size() + 1];
            for (EnvironmentMap::iterator it = envMap_.begin(); it != envMap_.end(); it++) {
                wstring envVariableW = it->first + L"=" + it->second;
                string envVariableA;
                StringUtility::Utf16ToUtf8(envVariableW, envVariableA);
                env[envIndex] = new char[envVariableA.length() + 1];
                memcpy(env[envIndex], envVariableA.c_str(), envVariableA.length());
                env[envIndex][envVariableA.length()] = 0;
                envIndex++;
            }
            env[envIndex] = 0;
        }

        string exeA;
        StringUtility::Utf16ToUtf8(processDescription_.ExePath, exeA);

        struct stat sb;
        if ((0 == stat(exeA.c_str(), &sb)) && !(sb.st_mode & S_IXUSR))
        {
            chmod(exeA.c_str(), sb.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
        }

        char** arg = nullptr;
        int argIndex = 0;
        arg = new char *[arguments_.size() + 2];
        arg[argIndex++] = (char*)exeA.c_str();
        for (string &argument : arguments_) {
            arg[argIndex] = new char[argument.length() + 1];
            memcpy(arg[argIndex], argument.c_str(), argument.length());
            arg[argIndex][argument.length()] = 0;
            argIndex++;
        }
        arg[argIndex] = 0;

        // set work directory
        string workdir;
        if(!processDescription_.StartInDirectory.empty())
        {
            StringUtility::Utf16ToUtf8(processDescription_.StartInDirectory, workdir);
        }

        string cgroupName;
        if (!processDescription_.CgroupName.empty())
        {
            StringUtility::Utf16ToUtf8(processDescription_.CgroupName, cgroupName);
        }

        uid_t uid = geteuid();
        if(runAs_!=nullptr && runAs_->get_Sid() && runAs_->get_Sid()->PSid)
        {
            uid = ((PLSID) (runAs_->get_Sid()->PSid))->SubAuthority[SID_SUBAUTH_ARRAY_UID];
        }

        ProcessActivatorThreadSingleton *pActivatorThread = ProcessActivatorThreadSingleton::GetSingleton();
        ErrorCode error = pActivatorThread->CreateProcess((char*)exeA.c_str(), arg, env,
          (char*)workdir.c_str(), uid, this->processDescription_.NotAttachedToJob,
          this->processExitCallback_, processId_, &(this->processDescription_.ResourceGovernancePolicy),
          &(this->processDescription_.ServicePackageResourceGovernance), cgroupName);
        return error;
    }

private:
    ProcessActivator & owner_;
    SecurityUserSPtr const runAs_;
    ProcessWait::WaitCallback const processExitCallback_;
    ProcessDescription const processDescription_;
    TimeoutHelper timeoutHelper_;
    pid_t processId_;
    EnvironmentMap envMap_;
    vector<string> arguments_;
    wstring fabricBinFolder_;

    int stdoutHandle_;
    int stderrHandle_;
    ProcessConsoleRedirectorSPtr stdoutRedirector_;
    ProcessConsoleRedirectorSPtr stderrRedirector_;
};

// ********************************************************************************************************************
// ProcessActivator::DeactivateAsyncOperation Implementation
//
class ProcessActivator::DeactivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        __in ProcessActivator & owner,
        IProcessActivationContextSPtr const & activationContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),   
        activationContext_(activationContext),
        timeoutHelper_(timeout),        
        isCancelled_(false)
    {
    }

    virtual ~DeactivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        pid_t pid = activationContext_->ProcessId;
        pid_t pgid = getpgid(pid);
        if (pgid > 0)
        {
            kill(-pgid, SIGINT);
        }
        else
        {
            kill(pid, SIGINT);
        }

        pwait_ = ProcessWait::CreateAndStart(
            pid,
            [this, thisSPtr] (pid_t pid, ErrorCode const& waitResult, DWORD)
            {
                OnProcessTerminated(thisSPtr, Common::Handle(NULL), waitResult);
            },
            timeoutHelper_.GetRemainingTime());
    }

    void OnProcessTerminated(AsyncOperationSPtr const & thisSPtr, Common::Handle const &, Common::ErrorCode const & waitResult)
    {
        bool isProcessTerminated = true;

        // If we timed out, there is a possibility that the process didn't shutdown as a result of Ctrl-C
        // We will force shutdown it using Terminate
        if (!waitResult.IsSuccess())
        {
            WriteNoise(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Process didn't shutdown as a result of CtrlC. Attempting to terminate the process");
            isProcessTerminated = false;
        }

        FinishTerminateProcess(thisSPtr, isProcessTerminated);
    }

    void FinishTerminateProcess(AsyncOperationSPtr const & thisSPtr, bool isProcessTerminated)
    {
        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Process {0} using CtrlC.", (isProcessTerminated ? L"terminated": L"did not terminate"));
        ErrorCode error = owner_.Cleanup(activationContext_, isProcessTerminated, ProcessActivator::ProcessDeactivateExitCode);
        TryComplete(thisSPtr, error);
    }

    void OnCancel()
    {
        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "ProcessActivator::DeactivateAsyncOperation: Cancel called.");

        isCancelled_ = true;
    }

private:
    ProcessActivator & owner_;
    IProcessActivationContextSPtr const activationContext_;
    TimeoutHelper timeoutHelper_;
    ExclusiveLock sharedPtrLock_;
    bool isCancelled_;
    ProcessWaitSPtr pwait_;
};

// ********************************************************************************************************************
// ProcessActivator Implementation
//

ProcessActivator::ProcessActivator(ComponentRoot const & root) : RootedObject(root)
{
}

ProcessActivator::~ProcessActivator()
{
}

AsyncOperationSPtr ProcessActivator::BeginActivate(
    SecurityUserSPtr const & runAs,
    ProcessDescription const & processDescription,
    wstring const & fabricbinPath,
    ProcessWait::WaitCallback const & processExitCallback,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    bool allowWinfabAdminSync)
{
    //LINUXTODO
    UNREFERENCED_PARAMETER(allowWinfabAdminSync);

    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        runAs,
        processDescription,
        fabricbinPath,
        processExitCallback,
        timeout,
        callback,
        parent);
}

ErrorCode ProcessActivator::EndActivate(
    AsyncOperationSPtr const & operation,
    __out IProcessActivationContextSPtr & activationContext)
{
    return ActivateAsyncOperation::End(operation, activationContext);
}

AsyncOperationSPtr ProcessActivator::BeginDeactivate(
    IProcessActivationContextSPtr const & activationContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        activationContext,        
        timeout,
        callback,
        parent);
}

ErrorCode ProcessActivator::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

ErrorCode ProcessActivator::UpdateRGPolicy(
    IProcessActivationContextSPtr & activationContext,
    ProcessDescriptionUPtr & processDescription)
{
    return ErrorCodeValue::Success;
}

ErrorCode ProcessActivator::Terminate(IProcessActivationContextSPtr const & activationContext, UINT uExitCode)
{
    return Cleanup(activationContext, false, uExitCode);
}

ErrorCode ProcessActivator::Cleanup(IProcessActivationContextSPtr const & activationContext, bool processClosedUsingCtrlC, UINT uExitCode)
{
    int result = 0;
    if (!processClosedUsingCtrlC)
    {
        WriteNoise(
                TraceType_Activator,
                "Killing Process Group of Process {0} with SIGKILL.", activationContext->ProcessId);

        pid_t pgid = getpgid(activationContext->ProcessId);
        if (pgid != getpgid(0))  // do not kill self
        {
            result = kill(-pgid, SIGKILL);
        }

        if(result != 0)
        {
            WriteError(
                    TraceType_Activator,
                    "Kill with SIGKILL failed for Process Group {0}, error {1}",
                    pgid,
                    errno);
        }
    }
    return activationContext->TerminateAndCleanup(!processClosedUsingCtrlC, uExitCode);
}

RwLock ProcessActivator::cgroupLock_;

int ProcessActivator::CreateCgroup(std::string const & cgroupName, const ResourceGovernancePolicyDescription * resourceGovernanceDescription, 
  const ServicePackageResourceGovernanceDescription * servicePackageResourceDescription, struct cgroup ** pCgroupObj, bool forSpOnly)
{
    AcquireExclusiveLock grab(cgroupLock_);

    struct cgroup * cgroupCP = nullptr;
    struct cgroup * cgroupSP = nullptr;
    int error = 0;
    std::string cgroupSPName;
    
    if (!forSpOnly)
    {
        //set up limits on CP level
        cgroupCP = cgroup_new_cgroup(cgroupName.c_str());
        if (cgroupCP)
        {
            auto memoryController = cgroup_add_controller(cgroupCP, "memory");
            if (memoryController)
            {   
                if (resourceGovernanceDescription->MemoryInMB > 0)
                {
                    string memoryLimit = to_string(resourceGovernanceDescription->MemoryInMB) + "M";
                    cgroup_add_value_string(memoryController, "memory.limit_in_bytes", memoryLimit.c_str());
                }
            }
            else
            {
                error = ECGFAIL;
                goto CgroupCreateCleanup;
            }
            auto cpuController = cgroup_add_controller(cgroupCP, "cpu");
            if (cpuController)
            {
                if (resourceGovernanceDescription->CpuQuota > 0)
                {
                    string cpuQuota = to_string(resourceGovernanceDescription->CpuQuota);
                    cgroup_add_value_string(cpuController, "cpu.cfs_quota_us", cpuQuota.c_str());
                    string cpuPeriod = to_string(Constants::CgroupsCpuPeriod);
                    cgroup_add_value_string(cpuController, "cpu.cfs_period_us", cpuPeriod.c_str());
                }
            }
            else
            {
                error = ECGFAIL;
                goto CgroupCreateCleanup;          
            }
            //this commits the cpu group object
            //this will also create all the parent cgroups if they do not exist yet
            error = cgroup_create_cgroup(cgroupCP, 0);
            if (error) goto CgroupCreateCleanup;
        }
        else
        {
            error = ECGFAIL;
            goto CgroupCreateCleanup;
        }
    }

    //setup limits on SP level
    if (servicePackageResourceDescription->IsGoverned)
    {
        if (forSpOnly)
        {
            cgroupSPName = cgroupName;
        }
        else
        {
            cgroupSPName = cgroupName.substr(0, cgroupName.find_last_of('/'));
        }
        cgroupSP = cgroup_new_cgroup(cgroupSPName.c_str());
        if (cgroupSP)
        {
            if (servicePackageResourceDescription->MemoryInMB > 0)
            {
                auto memoryController = cgroup_add_controller(cgroupSP, "memory"); 
                if (memoryController)
                {
                    string memoryLimit = to_string(servicePackageResourceDescription->MemoryInMB) + "M";
                    cgroup_add_value_string(memoryController, "memory.limit_in_bytes", memoryLimit.c_str());
                }
                else
                {
                    error = ECGFAIL;
                    goto CgroupCreateCleanup;
                }  
            }
            if (servicePackageResourceDescription->CpuCores > 0)
            {
                auto cpuController = cgroup_add_controller(cgroupSP, "cpu");
                if (cpuController)
                {
                    string cpuLimit = to_string((uint)(servicePackageResourceDescription->CpuCores * Constants::CgroupsCpuPeriod));
                    cgroup_add_value_string(cpuController, "cpu.cfs_quota_us", cpuLimit.c_str());
                    string cpuPeriod = to_string(Constants::CgroupsCpuPeriod);
                    cgroup_add_value_string(cpuController, "cpu.cfs_period_us", cpuPeriod.c_str());
                }
                else
                {
                    error = ECGFAIL;
                    goto CgroupCreateCleanup;
                }  
            }
            //if some controller is already existant this will succeed and just set the limits
            //there are cases where the CP does not have limits and SP does so we need to create controllers
            error = cgroup_create_cgroup(cgroupSP, 0);
            if (error) goto CgroupCreateCleanup;
        }
        else
        {
            error = ECGFAIL;
            goto CgroupCreateCleanup;
        }
        
    }
CgroupCreateCleanup:
    if (pCgroupObj && !error)
    {
        *pCgroupObj = cgroupCP;
    }
    else if (cgroupCP)
    {
        cgroup_free(&cgroupCP);
    }
    if (cgroupSP)
    {
        cgroup_free(&cgroupSP);
    }
    return error;
}

int ProcessActivator::RemoveCgroup(const std::string & cgroupName, bool forSpOnly)
{
    AcquireExclusiveLock grab(cgroupLock_);

    struct cgroup * cgroupCP = nullptr;
    struct cgroup * cgroupSP = nullptr;
    int error = 0;
    std::string cgroupSPName;

    if (!forSpOnly)
    {
        //remove CP cgroup
        cgroupCP = cgroup_new_cgroup(cgroupName.c_str());
        if (cgroupCP)
        {
            auto memoryController = cgroup_add_controller(cgroupCP, "memory");
            if (!memoryController) 
            {
                error = ECGFAIL;
                goto CgroupRemoveCleanup;
            }
            auto cpuController = cgroup_add_controller(cgroupCP, "cpu");
            if (!cpuController) 
            {
                error = ECGFAIL;
                goto CgroupRemoveCleanup;
            }
            error = cgroup_delete_cgroup_ext(cgroupCP, CGFLAG_DELETE_RECURSIVE | CGFLAG_DELETE_IGNORE_MIGRATION);
            if (error) goto CgroupRemoveCleanup;
        }
        else
        {
            error = ECGFAIL;
            goto CgroupRemoveCleanup;
        }
    }
    //try and remove SP - if this fails it means we have some other CP in this SP if forSpOnly is false
    //else try and remove recursively all cgroups in the hierarchy
    if (forSpOnly)
    {
        cgroupSPName = cgroupName;
    }
    else
    {
        cgroupSPName = cgroupName.substr(0, cgroupName.find_last_of('/'));
    }
    cgroupSP = cgroup_new_cgroup(cgroupSPName.c_str());
    if (cgroupSP)
    {
        auto memoryController = cgroup_add_controller(cgroupSP, "memory");
        if (!memoryController) 
        {
            error = ECGFAIL;
            goto CgroupRemoveCleanup;
        }
        auto cpuController = cgroup_add_controller(cgroupSP, "cpu");
        if (!cpuController) 
        {
            error = ECGFAIL;
            goto CgroupRemoveCleanup;
        }
        if (forSpOnly)
        {
            error = cgroup_delete_cgroup_ext(cgroupSP, CGFLAG_DELETE_RECURSIVE | CGFLAG_DELETE_IGNORE_MIGRATION);
        }
        else
        {
            //this one might be a fine situation in case there are more CP
            //anyhow keep the error code
            error = cgroup_delete_cgroup(cgroupSP, CGFLAG_DELETE_IGNORE_MIGRATION);
        } 
        if (error != 0) goto CgroupRemoveCleanup;
    }
    else
    {
        error = ECGFAIL;
        goto CgroupRemoveCleanup;
    }
CgroupRemoveCleanup:
     if (cgroupCP)
     {
        cgroup_free(&cgroupCP);
     }
     if (cgroupSP)
     {
        cgroup_free(&cgroupSP);
     }
     return error;
}

int ProcessActivator::Test_QueryCgroupInfo(std::string const & cgroupName, uint64 & cpuLimit, uint64 & memoryLimit)
{
    AcquireExclusiveLock grab(cgroupLock_);
    struct cgroup * cgroupCP = nullptr;
    int error = 0;

    cgroupCP = cgroup_new_cgroup(cgroupName.c_str());
    if (cgroupCP)
    {
        error = cgroup_get_cgroup(cgroupCP);
        if (error) goto CgroupQueryCleanup;

        auto memoryController = cgroup_get_controller(cgroupCP, "memory");
        if (!memoryController) 
        {
            error = ECGFAIL;
            goto CgroupQueryCleanup;
        }
        auto cpuController = cgroup_get_controller(cgroupCP, "cpu");
        if (!cpuController) 
        {
            error = ECGFAIL;
            goto CgroupQueryCleanup;
        }
        error = cgroup_get_value_uint64(memoryController, "memory.limit_in_bytes", &memoryLimit);
        if (error) goto CgroupQueryCleanup;
        error = cgroup_get_value_uint64(cpuController, "cpu.cfs_quota_us", &cpuLimit);
        //if these values are set it means we do not have governance actually
        if (cpuLimit >= UINT32_MAX)
        {
            cpuLimit = 0;
        }
        // 0x7FFFFFFFFFFFF000 - unlimited memory
        if (memoryLimit >= 0x7FFFFFFFFFFFF000)
        {
            memoryLimit = 0;
        }
        if (error) goto CgroupQueryCleanup;
    }
    else
    {
        error = ECGFAIL;
        goto CgroupQueryCleanup;
    }
CgroupQueryCleanup:
     if (cgroupCP)
     {
        cgroup_free(&cgroupCP);
     }
     return error;
}


int ProcessActivator::GetCgroupUsage(std::string const & cgroupName, uint64 & cpuUsage, uint64 & memoryUsage)
{
    AcquireExclusiveLock grab(cgroupLock_);
    struct cgroup * cgroupCP = nullptr;
    int error = 0;

    cgroupCP = cgroup_new_cgroup(cgroupName.c_str());
    if (cgroupCP)
    {
        error = cgroup_get_cgroup(cgroupCP);
        if (error) goto CgroupQueryCleanup;

        auto memoryController = cgroup_get_controller(cgroupCP, "memory");
        if (!memoryController) 
        {
            error = ECGFAIL;
            goto CgroupQueryCleanup;
        }
        auto cpuController = cgroup_get_controller(cgroupCP, "cpuacct");
        if (!cpuController) 
        {
            error = ECGFAIL;
            goto CgroupQueryCleanup;
        }
        error = cgroup_get_value_uint64(memoryController, "memory.usage_in_bytes", &memoryUsage);
        if (error) goto CgroupQueryCleanup;
        
        error = cgroup_get_value_uint64(cpuController, "cpuacct.usage", &cpuUsage);
        
        if (error) goto CgroupQueryCleanup;
    }
    else
    {
        error = ECGFAIL;
        goto CgroupQueryCleanup;
    }
CgroupQueryCleanup:
     if (cgroupCP)
     {
        cgroup_free(&cgroupCP);
     }
     return error;
}
