// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define __null 0
#undef _WIN64
#undef WIN32_LEAN_AND_MEAN
#include <grpc++/grpc++.h>
#include "cri-api.grpc.pb.h"
#define _WIN64 1
#define WIN32_LEAN_AND_MEAN 1
#include "stdafx.h"
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <libgen.h>
#include "sys/prctl.h"
#include <sys/wait.h>
#include <resolv.h>
#include <arpa/inet.h>
#include "ContainerRuntimeHandler.h"

using namespace std;
using namespace Hosting2;
using namespace Common;
using namespace ServiceModel;

using namespace runtime;

namespace {
    StringLiteral TraceType_ContainerRuntimeHandler = "ContainerRuntimeHandler";
    const char *CrioSocketPath = "/var/run/crio/crio.sock";
    const char *CrioReadyPath = "/var/run/crio/crio.ready";
    const char *FabricDataRootFile = "/etc/servicefabric/FabricDataRoot";
}

// class static variables
string ContainerRuntimeHandler::ContainerLogRoot = "/var/log/sflogs/";

std::map<void*, void*> ContainerRuntimeHandler::ContainerStatusCallbacks;
std::map<void*, void*> ContainerRuntimeHandler::PodSandboxStatusCallbacks;
Common::RwLock ContainerRuntimeHandler::CrioNameMapLock;
ContainerRuntimeHandler *ContainerRuntimeHandler::ContainerRuntimeHandlerSingleton = nullptr;
Common::RwLock ContainerRuntimeHandler::InitializationLock;
Common::RwLock ContainerRuntimeHandler::SingletonObjectLock;
ContainerRuntimeHandler::InitializationState ContainerRuntimeHandler::InitState(InitializationState::Uninitialized);
vector<AsyncOperationSPtr> *ContainerRuntimeHandler::PendingOperations = new vector<AsyncOperationSPtr>();

std::map<string, string> ContainerRuntimeHandler::PodSandboxNameToIdMap;
std::map<string, string> ContainerRuntimeHandler::ContainerNameToIdMap;
std::map<string, ContainerRuntimeHandler::PodSandboxInfo> ContainerRuntimeHandler::PodSandboxTbl;
std::map<string, ContainerRuntimeHandler::ContainerInfo> ContainerRuntimeHandler::ContainerTbl;

std::atomic_int ContainerRuntimeHandler::InitializationThreadStatus(0);
std::atomic_int ContainerRuntimeHandler::StatePollThreadStatus(0);
std::atomic_int ContainerRuntimeHandler::CompletionQueueThreadStatus(0);

int ContainerRuntimeHandler::InitializationThreadIteration = 0;

ContainerRuntimeHandler* ContainerRuntimeHandler::GetSingleton()
{
    AcquireReadLock lock(SingletonObjectLock);
    ASSERT_IF(!ContainerRuntimeHandlerSingleton, "Failed to initialize Crio ContainerRuntimeHandler");
    return ContainerRuntimeHandlerSingleton;
}

// ********************************************************************************************************************
// Initialization Implementation
//

pid_t ContainerRuntimeHandler::ExecWithPrctl(string const & exePath, vector<string> const & args,
                                             vector<string> const & envs, ProcessWait::WaitCallback exitCallback)
{
    pid_t ppid = getpid();
    pid_t pID = fork();
    if (pID == 0)  // child
    {
        // make it session leader
        setsid();

        // setup pseudo tty
        int fdm, fds;
        char* pts_name;
        fdm = posix_openpt(O_RDWR);
        if (fdm < 0 || unlockpt(fdm) < 0 || (pts_name = ptsname(fdm)) == 0
            || (fds = open(pts_name, O_RDWR)) == 0  || ioctl(fds, TIOCSCTTY, NULL) < 0)
        {
            abort();
        }

        // args
        unique_ptr<const char*[]> argv(new const char*[args.size() + 2]);
        for (int i = 0; i < args.size() + 2; i++)
        {
            if (i == 0)
            {
                argv[0] = exePath.c_str();
            }
            else if (i == args.size() + 1)
            {
                argv[i] = 0;
            }
            else
            {
                argv[i] = args[i - 1].c_str();
            }
        }

        // envs
        unique_ptr<const char*[]> envp(new const char*[envs.size() + 1]);
        envp[envs.size()] = 0;
        for (int i = 0; i < envs.size(); i++)
        {
            envp[i] = envs[i].c_str();
        }

        // prctl
        prctl(PR_SET_PDEATHSIG, SIGUSR1);

        // execv
        auto retv=execve(exePath.c_str(), (char**)argv.get(), (char**)envp.get());

        WriteWarning(TraceType_ContainerRuntimeHandler, "execv failed. err={0}.", errno);
        abort();
    }
    // parent
    if (pID < 0)  // failed to fork
    {
        WriteWarning(TraceType_ContainerRuntimeHandler, "fork failed. err={0}.", errno);
        return pID;
    }

    // exit monitoring
    ProcessWaitSPtr pwait;
    ProcessWait::Setup();
    pwait = ProcessWait::Create();
    pwait->StartWait(
            pID,
            [pwait, exitCallback](pid_t pid, ErrorCode const & waitResult, DWORD exitCode)
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "wait callback called for process {0}, waitResult={1}, exitCode={2}", pid, waitResult, exitCode);
                exitCallback(pid, waitResult, exitCode);
            });
    return pID;
}

void ContainerRuntimeHandler::CrioProcessMonitor(pid_t pid, Common::ErrorCode const & waitResult, DWORD processExitCode)
{
    WriteInfo(TraceType_ContainerRuntimeHandler, "crio.sh exited.");
}

void* ContainerRuntimeHandler::InitializationThread(void *arg)
{
    InitializationThreadStatus.store(UtilityThreadState::ThreadRunning);
    const int IterationMax = 100000000;
    const int CrioConseqFailuresMax = 50;
    int crioConseqFailures = 0;

    while (InitializationThreadStatus.load() != UtilityThreadState::ThreadCancelling)
    {
        // main iterations
        bool crioStarted = InitializationThreadLoop(InitializationThreadIteration);
        InitializationThreadIteration = (InitializationThreadIteration + 1) % IterationMax;
        crioConseqFailures = (crioStarted ? 0 : crioConseqFailures + 1);

        // too many failures
        if (crioConseqFailures >= CrioConseqFailuresMax)
        {
            WriteWarning(TraceType_ContainerRuntimeHandler,
                         "Failed to start crio after {0} retrying. ContainerRuntimeHandler exits.", CrioConseqFailuresMax);

            // notify hosting that all pods/containers are gone
            {
                AcquireWriteLock lock(CrioNameMapLock);
                for (auto pod = PodSandboxTbl.begin(); pod != PodSandboxTbl.end(); )
                {
                    WriteInfo(TraceType_ContainerRuntimeHandler, "PodSandbox Exited(DISAPPEAR), id: {0}, apphost: {1}",
                              pod->second.id_, pod->second.apphost_id_);
                    for (auto callback: PodSandboxStatusCallbacks)
                    {
                        string apphostid = pod->second.apphost_id_;
                        string nodeid = pod->second.node_id_;
                        string metaname = pod->second.meta_name_;
                        PodSandboxState state = pod->second.state_;
                        PodSandboxState nstate = PodSandboxState::NotReady;
                        Threadpool::Post([callback, apphostid, nodeid, metaname, state, nstate] {
                            (*((PodSandboxStateUpdateCallback)(callback.first)))
                                    (apphostid, nodeid, metaname, state, nstate, callback.second);
                        });
                    }
                    pod = PodSandboxTbl.erase(pod);
                }
                for (auto container = ContainerTbl.begin(); container != ContainerTbl.end(); )
                {
                    WriteInfo(TraceType_ContainerRuntimeHandler, "Container Exited (DISAPPEAR), id: {0}, apphost: {1}",
                              container->second.id_, container->second.apphost_id_);
                    for (auto callback: ContainerStatusCallbacks)
                    {
                        string apphostid = container->second.apphost_id_;
                        string nodeid = container->second.node_id_;
                        string metaname = container->second.meta_name_;
                        ContainerState state = container->second.state_;
                        ContainerState nstate = ContainerState::Exited;
                        Threadpool::Post([callback, apphostid, nodeid, metaname, state, nstate] {
                            (*((ContainerStateUpdateCallback)(callback.first)))
                                    (apphostid, nodeid, metaname, state, nstate, callback.second);
                        });
                    }
                    container = ContainerTbl.erase(container);
                }
            }

            // cleanup pending ops (could be cascading)
            int quiescenceTime = 32;
            while (quiescenceTime--)
            {
                CleanPendingOperations(ErrorCodeValue::OperationFailed);
                Sleep(500);
            }
            break;
        }
    }

    InitializationThreadStatus.store(UtilityThreadState::ThreadNone);
    {
        AcquireWriteLock lock(InitializationLock);
        InitState = InitializationState::Uninitialized;
    }
    return 0;
}

void ContainerRuntimeHandler::CleanPendingOperations(ErrorCode const & error)
{
    vector<AsyncOperationSPtr> ops;
    {
        AcquireWriteLock lock(InitializationLock);
        ops = *PendingOperations;
        PendingOperations->clear();
    }
    for (auto op: ops)  op->TryComplete(op, error);
}

bool ContainerRuntimeHandler::InitializationThreadLoop(int iteration)
{
    string fabricCrioSock = string(CrioSocketPath) + string(".") + std::to_string(iteration);
    WriteInfo(TraceType_ContainerRuntimeHandler, "Entering InitializationThreadLoop {0}", iteration);
    BOOL success = TRUE;
    pid_t pid;
    do {
        wstring fabricBinRootW, fabricDataRootW, fabricLogRootW;
        auto error = FabricEnvironment::GetFabricDataRoot(fabricDataRootW);
        if (!(success = error.IsSuccess()))  break;

        error = FabricEnvironment::GetFabricBinRoot(fabricBinRootW);
        if (!(success = error.IsSuccess()))  break;

        error = FabricEnvironment::GetFabricLogRoot(fabricLogRootW);
        if (!(success = error.IsSuccess()))  break;

        wstring fabricContainerLogRootW = Path::Combine(fabricLogRootW, L"Containers");
        if (!(success = Directory::Exists(fabricContainerLogRootW)))
        {
            WriteWarning(TraceType_ContainerRuntimeHandler, "fabricContainerLogRoot doen't exist {0}", fabricContainerLogRootW);
            break;
        }
        string fabricBinRoot,fabricDataRoot, fabricLogRoot, fabricContainerLogRoot;
        StringUtility::Utf16ToUtf8(fabricBinRootW, fabricBinRoot);
        StringUtility::Utf16ToUtf8(fabricDataRootW, fabricDataRoot);
        StringUtility::Utf16ToUtf8(fabricLogRootW, fabricLogRoot);
        StringUtility::Utf16ToUtf8(fabricContainerLogRootW, fabricContainerLogRoot);
        ContainerLogRoot = fabricContainerLogRoot + "/";

        string crioPath;
        string crioConfigFile;
        if (HostingConfig::GetConfig().UseKataContainerRuntime) {
            crioPath = fabricBinRoot + "/Fabric/Fabric.Code/crio.kata.sh";
            crioConfigFile = "crio.kata.conf";
        } else {
            crioPath = fabricBinRoot + "/Fabric/Fabric.Code/crio.cc.sh";
            crioConfigFile = "crio.cc.conf";
        }

        vector<string> crioArgs = { crioPath };

        const char* pathEnv = getenv("PATH");
        const char* libEnv = getenv("LD_LIBRARY_PATH");
        vector<string> crioEnvs = { string("FabricDataRoot=") + fabricDataRoot,
                                    string("FabricBinRoot=") + fabricBinRoot,
                                    string("FabricLogRoot=") + fabricLogRoot,
                                    string("FabricContainerLogRoot=") + fabricContainerLogRoot,
                                    string("FabricCrioSock=") + fabricCrioSock,
                                    string("FabricCrioConfig=") + crioConfigFile};
        if (pathEnv) crioEnvs.push_back(string("PATH=") + pathEnv);
        if (libEnv) crioEnvs.push_back(string("LD_LIBRARY_PATH=") + libEnv);

        WriteInfo(TraceType_ContainerRuntimeHandler,
                  "execve {0}. fabricBinRoot {1}, fabricDataRoot {2}",
                  crioPath, fabricBinRoot, fabricDataRoot);

        string rmready("rm ");
        rmready += CrioReadyPath;
        system(rmready.c_str());

        if (!(success = (0 < (pid = ExecWithPrctl(crioPath, crioArgs, crioEnvs, CrioProcessMonitor))))) break;

        int fd = socket(PF_UNIX, SOCK_STREAM, 0);
        if (!(success = (fd >= 0))) break;

        struct stat readystat;
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, fabricCrioSock.c_str(), sizeof(addr.sun_path) - 1);
        int retry = 1200;
        int interval = 500;
        while (retry && !(success = (0 == stat(CrioReadyPath, &readystat))))
        {
            if (0 != kill(pid, 0))
            {
                success = false; close(fd); break;
            }
            Sleep(interval);
            retry--;
        }
        success = (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0);
        close(fd);
    } while (false);

    if (success)
    {
        auto channel = grpc::CreateChannel(string("unix:")+fabricCrioSock, grpc::InsecureChannelCredentials());
        ContainerRuntimeHandlerSingleton = new ContainerRuntimeHandler(channel);
        Invariant(ContainerRuntimeHandlerSingleton != 0);

        pthread_t tid;
        pthread_attr_t pthreadAttr;
        Invariant(pthread_attr_init(&pthreadAttr) == 0);
        Invariant(pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED) == 0);

        CompletionQueueThreadStatus.store(UtilityThreadState::ThreadCreated);
        auto retval = pthread_create(&tid, &pthreadAttr, &ContainerRuntimeHandler::CompletionQueueThread, 0);
        Invariant(retval == 0);

        StatePollThreadStatus.store(UtilityThreadState::ThreadCreated);
        retval = pthread_create(&tid, &pthreadAttr, &ContainerRuntimeHandler::StatePollThread, 0);
        Invariant(retval == 0);

        pthread_attr_destroy(&pthreadAttr);

        {
            AcquireWriteLock lock(InitializationLock);
            InitState = InitializationState::Initialized;
        }
    }

    CleanPendingOperations(success && !iteration ? ErrorCodeValue::Success : ErrorCodeValue::OperationFailed);

    // keep running
    if (success)
    {
        while (0 == kill(pid, 0)) Sleep(1000);
    }
    else if (pid > 0)
    {
        kill(pid, SIGKILL);
    }

    // exiting, requests to queue
    {
        AcquireWriteLock lock(InitializationLock);
        InitState = InitializationState::Initializing;
    }

    if (ContainerRuntimeHandlerSingleton)
    {
        if (StatePollThreadStatus.load() != UtilityThreadState::ThreadNone)
        {
            StatePollThreadStatus.store(UtilityThreadState::ThreadCancelling);
            while (StatePollThreadStatus.load() != UtilityThreadState::ThreadNone) Sleep(100);
        }

        int quiescenceTime = 32;
        while (quiescenceTime--)  Sleep(500);

        if (CompletionQueueThreadStatus.load() != UtilityThreadState::ThreadNone)
        {
            CompletionQueueThreadStatus.store(UtilityThreadState::ThreadCancelling);
            while (CompletionQueueThreadStatus.load() != UtilityThreadState::ThreadNone) Sleep(100);
        }
        delete ContainerRuntimeHandlerSingleton;
        ContainerRuntimeHandlerSingleton = 0;
    }

    WriteInfo(TraceType_ContainerRuntimeHandler, "Exiting InitializationThreadLoop {0}", iteration);
    return success;
}

// ********************************************************************************************************************
// State polling and monitoring Implementation
//

void* ContainerRuntimeHandler::StatePollThread(void *arg)
{
    StatePollThreadStatus.store(UtilityThreadState::ThreadRunning);
    WriteInfo(TraceType_ContainerRuntimeHandler, "Entering StatePollThread");

    const int ListWaitTimeout = 600;  // 10 minutes

    while (StatePollThreadStatus.load() != UtilityThreadState::ThreadCancelling)
    {
        // podsandbox
        {
            shared_ptr<ManualResetEvent> evt = make_shared<ManualResetEvent>();
            weak_ptr<ManualResetEvent> weak_evt(evt);
            struct timespec timestamp;
            clock_gettime(CLOCK_REALTIME, &timestamp);
            WriteNoise(TraceType_ContainerRuntimeHandler, "BeginListPodSandbox");
            auto op = ContainerRuntimeHandler::BeginListPodSandbox(
                    L"ContainerStatePollThread", TimeSpan::MaxValue,
                    [weak_evt](AsyncOperationSPtr const & operationSPtr) -> void
                    {
                        auto ptr = weak_evt.lock();
                        if (ptr) ptr->Set();
                    },
                    AsyncOperationSPtr());
            int waittime = ListWaitTimeout;
            while (!evt->WaitOne(1000) && StatePollThreadStatus.load() != UtilityThreadState::ThreadCancelling && --waittime);
            if (StatePollThreadStatus.load() == UtilityThreadState::ThreadCancelling)
            {
                op->Cancel(true); break;
            }
            if (!waittime)
            {
                op->Cancel(true); continue;
            }

            vector<runtime::PodSandbox> itemsvec;
            WriteNoise(TraceType_ContainerRuntimeHandler, "EndListPodSandbox");
            ErrorCode err = ContainerRuntimeHandler::EndListPodSandbox(op, itemsvec);
            if (!err.IsSuccess()) continue;

            string idList;
            for (auto it : itemsvec)  idList += string(it.id()) + " ";
            WriteNoise(TraceType_ContainerRuntimeHandler,
                       "StatePollThread - PodSandbox {0} in total: {1}",
                       itemsvec.size(), idList);

            unordered_map<string, runtime::PodSandbox> items;
            for (auto it : itemsvec)
            {
                items.emplace(it.id(), it);
            }

            AcquireWriteLock lock(CrioNameMapLock);
            for (auto pod = PodSandboxTbl.begin(); pod != PodSandboxTbl.end(); )
            {
                auto item = items.find(pod->first);
                if (item == items.end())
                {
                    if (timestamp <= pod->second.create_timestamp_)
                    {
                        pod++; continue;
                    }

                    WriteInfo(TraceType_ContainerRuntimeHandler, "PodSandbox Exited(DISAPPEAR), id: {0}, apphost: {1}",
                              pod->second.id_, pod->second.apphost_id_);
                    for (auto callback: PodSandboxStatusCallbacks)
                    {
                        string apphostid = pod->second.apphost_id_;
                        string nodeid = pod->second.node_id_;
                        string metaname = pod->second.meta_name_;
                        PodSandboxState state = pod->second.state_;
                        PodSandboxState nstate = PodSandboxState::NotReady;

                        Threadpool::Post([callback, apphostid, nodeid, metaname, state, nstate] {
                            (*((PodSandboxStateUpdateCallback)(callback.first)))
                                    (apphostid, nodeid, metaname, state, nstate, callback.second);
                        });
                    }
                    pod = PodSandboxTbl.erase(pod);
                    continue;
                }
                else if (item->second.state() != pod->second.state_)
                {
                    if (item->second.state() == runtime::PodSandboxState::SANDBOX_NOTREADY)
                    {
                        WriteInfo(TraceType_ContainerRuntimeHandler, "PodSandbox Exited(NOT-READY), id: {0}, apphost: {1}",
                                  pod->second.id_, pod->second.apphost_id_);
                    }

                    for (auto callback: PodSandboxStatusCallbacks)
                    {
                        string apphostid = pod->second.apphost_id_;
                        string nodeid = pod->second.node_id_;
                        string metaname = pod->second.meta_name_;
                        PodSandboxState state = pod->second.state_;
                        PodSandboxState nstate = (PodSandboxState)item->second.state();

                        Threadpool::Post([callback, apphostid, nodeid, metaname, state, nstate] {
                            (*((PodSandboxStateUpdateCallback)(callback.first)))
                                    (apphostid, nodeid, metaname, state, nstate, callback.second);
                        });
                    }
                }
                pod->second.Update(item->second);
                pod++;
            }
        }

        // containers
        {
            shared_ptr<ManualResetEvent> evt = make_shared<ManualResetEvent>();
            weak_ptr<ManualResetEvent> weak_evt(evt);
            struct timespec timestamp;
            WriteNoise(TraceType_ContainerRuntimeHandler, "BeginListContainers");
            clock_gettime(CLOCK_REALTIME, &timestamp);
            auto op = ContainerRuntimeHandler::BeginListContainers(
                    std::map<std::wstring, std::wstring>(),
                    L"ContainerStatePollThread", TimeSpan::MaxValue,
                    [weak_evt](AsyncOperationSPtr const & operationSPtr) -> void
                    {
                        auto ptr = weak_evt.lock();
                        if (ptr) ptr->Set();
                    },
                    AsyncOperationSPtr());
            int waittime = ListWaitTimeout;
            while (!evt->WaitOne(1000) && StatePollThreadStatus.load() != UtilityThreadState::ThreadCancelling && --waittime);
            if (StatePollThreadStatus.load() == UtilityThreadState::ThreadCancelling)
            {
                op->Cancel(true); break;
            }
            if (!waittime)
            {
                op->Cancel(true); continue;
            }

            vector<runtime::Container> itemsvec;
            WriteNoise(TraceType_ContainerRuntimeHandler, "EndListContainers");
            ErrorCode err = ContainerRuntimeHandler::EndListContainers(op, itemsvec);
            if (!err.IsSuccess()) continue;

            string idList;
            for (auto it : itemsvec)  idList += string(it.id()) + " ";
            WriteNoise(TraceType_ContainerRuntimeHandler,
                       "StatePollThread - Container {0} in total: {1}",
                       itemsvec.size(), idList);

            unordered_map<string, runtime::Container> items;
            for (auto it : itemsvec)
            {
                items.emplace(it.id(), it);
            }

            AcquireWriteLock lock(CrioNameMapLock);

            for (auto container = ContainerTbl.begin(); container != ContainerTbl.end(); )
            {
                auto item = items.find(container->first);
                if (item == items.end())
                {
                    if (timestamp <= container->second.create_timestamp_)
                    {
                        container++; continue;
                    }

                    WriteInfo(TraceType_ContainerRuntimeHandler, "Container Exited (DISAPPEAR), id: {0}, apphost: {1}",
                              container->second.id_, container->second.apphost_id_);

                    for (auto callback: ContainerStatusCallbacks)
                    {
                        string apphostid = container->second.apphost_id_;
                        string nodeid = container->second.node_id_;
                        string metaname = container->second.meta_name_;
                        ContainerState state = container->second.state_;
                        ContainerState nstate = ContainerState::Exited;

                        Threadpool::Post([callback, apphostid, nodeid, metaname, state, nstate] {
                            (*((ContainerStateUpdateCallback)(callback.first)))
                                    (apphostid, nodeid, metaname, state, nstate, callback.second);
                        });
                    }
                    container = ContainerTbl.erase(container);
                    continue;
                }
                else if (item->second.state() != container->second.state_)
                {
                    if (item->second.state() == runtime::ContainerState::CONTAINER_EXITED
                         || item->second.state() == runtime::ContainerState::CONTAINER_UNKNOWN)
                    {
                        WriteInfo(TraceType_ContainerRuntimeHandler, "Container Exited (EXIT), id: {0}, apphost: {1}",
                                  container->second.id_, container->second.apphost_id_);
                    }

                    for (auto callback: ContainerStatusCallbacks)
                    {
                        string apphostid = container->second.apphost_id_;
                        string nodeid = container->second.node_id_;
                        string metaname = container->second.meta_name_;
                        ContainerState state = container->second.state_;
                        ContainerState nstate = (ContainerState)item->second.state();

                        Threadpool::Post([callback, apphostid, nodeid, metaname, state, nstate] {
                            (*((ContainerStateUpdateCallback)(callback.first)))
                                    (apphostid, nodeid, metaname, state, nstate, callback.second);
                        });
                    }
                }
                container->second.Update(item->second);
                container++;
            }
        }
        Sleep(5000);
    }
    WriteInfo(TraceType_ContainerRuntimeHandler, "Exiting StatePollThread");
    StatePollThreadStatus.store(UtilityThreadState::ThreadNone);
    return 0;
}

void* ContainerRuntimeHandler::RegisterContainerStateUpdateCallback(ContainerRuntimeHandler::ContainerStateUpdateCallback func, void *arg)
{
    AcquireWriteLock lock(CrioNameMapLock);
    ContainerStatusCallbacks[(void*)func] = arg;
    return (void*)func;
}

void ContainerRuntimeHandler::UnregisterContainerStateUpdateCallback(void *func)
{
    AcquireWriteLock lock(CrioNameMapLock);
    ContainerStatusCallbacks.erase(func);
}

void* ContainerRuntimeHandler::RegisterPodSandboxStateUpdateCallback(ContainerRuntimeHandler::PodSandboxStateUpdateCallback func, void *arg)
{
    AcquireWriteLock lock(CrioNameMapLock);
    PodSandboxStatusCallbacks[(void*)func] = arg;
    return (void*)func;
}

void ContainerRuntimeHandler::UnregisterPodSandboxStateUpdateCallback(void *func)
{
    AcquireWriteLock lock(CrioNameMapLock);
    PodSandboxStatusCallbacks.erase(func);
}

bool ContainerRuntimeHandler::PodSandboxInfo::Update(const runtime::PodSandbox & pod)
{
    bool trans = (state_ != pod.state());
    id_ = pod.id();
    meta_name_ = pod.metadata().name();
    meta_namespace_ = pod.metadata().namespace_();
    meta_attempt_ = pod.metadata().attempt();
    created_at_ = pod.created_at();
    state_ = (PodSandboxState) pod.state();
    return trans;
}

void ContainerRuntimeHandler::ContainerInfo::Update(const runtime::Container & ct)
{
    ContainerState nstate = (ContainerState)ct.state();
    id_ = ct.id();
    pod_sandbox_id_ = ct.pod_sandbox_id();
    meta_name_ = ct.metadata().name();
    meta_attempt_ = ct.metadata().attempt();
    created_at_ = ct.created_at();
    image_ref_ = ct.image_ref();
    state_ = nstate;
}

bool ContainerRuntimeHandler::GetPodIdFromName(string const & name, string &id)
{
    AcquireReadLock lock(CrioNameMapLock);
    if (PodSandboxNameToIdMap.count(name) != 0)
    {
        id = PodSandboxNameToIdMap[name];
        return true;
    }
    return false;
}

bool ContainerRuntimeHandler::GetContainerIdFromName(string const & name, string &id)
{
    AcquireReadLock lock(CrioNameMapLock);
    if (ContainerNameToIdMap.count(name) != 0)
    {
        id = ContainerNameToIdMap[name];
        return true;
    }
    return false;
}

void ContainerRuntimeHandler::PutPodSandboxInfo(string const &name, string const &id, string const &apphostid, string const &nodeid, string const &logdirectory)
{
    AcquireWriteLock lock(CrioNameMapLock);
    PodSandboxNameToIdMap[name] = id;
    PodSandboxTbl[id].id_ = id;
    PodSandboxTbl[id].meta_name_ = name;
    PodSandboxTbl[id].apphost_id_ = apphostid;
    PodSandboxTbl[id].node_id_ = nodeid;
    PodSandboxTbl[id].log_directory_ = logdirectory;

    clock_gettime(CLOCK_REALTIME, &PodSandboxTbl[id].create_timestamp_);
}

void ContainerRuntimeHandler::DelPodSandboxInfo(string const &name, string const &id)
{
    AcquireWriteLock lock(CrioNameMapLock);
    PodSandboxNameToIdMap.erase(name);
    PodSandboxTbl.erase(id);
}

void ContainerRuntimeHandler::PutContainerInfo(string const &name, string const &id, string const &apphostid, string const &nodeid, string const &logpath)
{
    AcquireWriteLock lock(CrioNameMapLock);
    ContainerNameToIdMap[name] = id;
    ContainerTbl[id].id_ = id;
    ContainerTbl[id].meta_name_ = name;
    ContainerTbl[id].apphost_id_ = apphostid;
    ContainerTbl[id].node_id_ = nodeid;
    ContainerTbl[id].log_path_ = logpath;

    clock_gettime(CLOCK_REALTIME, &ContainerTbl[id].create_timestamp_);
}

void ContainerRuntimeHandler::DelContainerInfo(string const &name, string const &id)
{
    AcquireWriteLock lock(CrioNameMapLock);
    ContainerNameToIdMap.erase(name);
    ContainerTbl.erase(id);
}

bool ContainerRuntimeHandler::GetContainerLogPath(string const &containername, string &path)
{
    AcquireWriteLock lock(CrioNameMapLock);
    auto cit = ContainerNameToIdMap.find(containername);
    if ( cit != ContainerNameToIdMap.end())
    {
        string containerLogPath = ContainerTbl[cit->second].log_path_;
        string podid = ContainerTbl[cit->second].pod_sandbox_id_;
        auto podit = PodSandboxTbl.find(podid);
        if (podit != PodSandboxTbl.end())
        {
            path = Path::Combine(podit->second.log_directory_, containerLogPath);
            return true;
        }
    }
    return false;
}

bool ContainerRuntimeHandler::PodExists(wstring const & podName)
{
    string podNameA = Common::StringUtility::Utf16ToUtf8(podName);
    AcquireWriteLock lock(CrioNameMapLock);
    return (PodSandboxNameToIdMap.find(podNameA) != PodSandboxNameToIdMap.end());
}

bool ContainerRuntimeHandler::ContainerExists(wstring const & containerName)
{
    string containerNameA = Common::StringUtility::Utf16ToUtf8(containerName);
    AcquireWriteLock lock(CrioNameMapLock);
    return (ContainerNameToIdMap.find(containerNameA) != ContainerNameToIdMap.end());
}

// ********************************************************************************************************************
// Async operations Implementation: Initialization, RunPodSandbox, CreateContainer, etc.
//

runtime::ContainerConfig* ContainerRuntimeHandler::genContainerConfig(
        string const & name, string const & imgname,
        vector<string> const & cmds, vector<string> const & args, vector<pair<string, string>> const & envs,
        vector<tuple<string, string, bool>> const & mounts, vector<pair<string, string>> const & labels,
        string const & workdir, string const & logpath, bool interactive)
{
    runtime::ContainerConfig *pconfig = new runtime::ContainerConfig();

    ContainerMetadata *pmeta = new ContainerMetadata();
    {
        pmeta->set_name(name);
        pmeta->set_attempt(0);
    }
    pconfig->set_allocated_metadata(pmeta);

    ImageSpec *pimgspec = new ImageSpec();
    {
        pimgspec->set_image(imgname);
    }
    pconfig->set_allocated_image(pimgspec);
    for (string cmd : cmds)
    {
        pconfig->add_command(cmd);
    }
    for (string arg : args)
    {
        pconfig->add_args(arg);
    }
    for (auto env : envs)
    {
        runtime::KeyValue *kv = pconfig->add_envs();
        kv->set_key(env.first);
        kv->set_value(env.second);
    }
    for (auto mntspec : mounts)
    {
        runtime::Mount *mnt = pconfig->add_mounts();
        mnt->set_host_path(std::get<0>(mntspec));
        mnt->set_container_path(std::get<1>(mntspec));
        mnt->set_readonly(std::get<2>(mntspec));
    }
    auto& labelsDictPtr = *pconfig->mutable_labels();
    for (auto label : labels)
    {
        labelsDictPtr[label.first] = label.second;
    }
    pconfig->clear_devices();
    pconfig->clear_annotations();

    if (workdir.empty())
    {
        pconfig->clear_working_dir();
    }
    else
    {
        pconfig->set_working_dir(workdir);
    }

    if (logpath.empty())
    {
        pconfig->clear_log_path();
    }
    else
    {
        pconfig->set_log_path(logpath);
    }

    if (interactive)
    {
        pconfig->set_stdin(true);
        pconfig->set_stdin_once(true);
        pconfig->set_tty(true);
    }

    LinuxContainerConfig *plinux = new LinuxContainerConfig();
    {
        plinux->clear_resources();
        LinuxContainerSecurityContext *plinuxsec = new LinuxContainerSecurityContext();
        {
            plinuxsec->clear_capabilities();
            plinuxsec->set_privileged(false);
            NamespaceOption *pnsoption = new NamespaceOption();
            {
                pnsoption->set_host_network(true);
                pnsoption->set_host_pid(true);
                pnsoption->set_host_ipc(true);
            }
            plinuxsec->set_allocated_namespace_options(pnsoption);
            plinuxsec->clear_selinux_options();
            plinuxsec->clear_run_as_user();
            plinuxsec->clear_run_as_username();
            plinuxsec->set_readonly_rootfs(false);
            plinuxsec->clear_supplemental_groups();
            plinuxsec->clear_apparmor_profile();
            plinuxsec->clear_seccomp_profile_path();
            plinuxsec->set_no_new_privs(false);
        }
        plinux->set_allocated_security_context(plinuxsec);
    }
    pconfig->set_allocated_linux(plinux);

    return pconfig;
}

runtime::PodSandboxConfig* ContainerRuntimeHandler::genPodSandboxConfig(std::string  const & name, 
                                                            std::string const & cgparent,
                                                            string const & hostname,
                                                            string const & logdirectory,
                                                            ServiceModel::NetworkType::Enum networkType,
                                                            bool isdnsserviceenabled,
                                                            string const & dnssearchoptions,
                                                            vector<string> & dnsServers,
                                                            vector<tuple<string, int, int, int>> const & portmappings)
{
    PodSandboxConfig *pconfig = new PodSandboxConfig();
    PodSandboxMetadata *pmeta = new PodSandboxMetadata();
    {
        pmeta->set_name(name);
        pmeta->set_uid("sfuser");
        pmeta->set_namespace_(string("ns_") + name);
        pmeta->set_attempt(5);
    }
    pconfig->set_allocated_metadata(pmeta);
    if (!hostname.empty())
    {
        pconfig->set_hostname(hostname);
    }
    pconfig->set_log_directory(logdirectory);

    pconfig->clear_dns_config();

    if (isdnsserviceenabled)
    {
        // If the dnsservice is enabled, we set the nameserver to host ip on cni bridge network (for nat and isolated network type) 
        // or host ip of the vm (for open network). The domain is set to the application name. ndots value is explicitly specified
        // to avoid any issues arising from the default behavior of clear container run time. 
        runtime::DNSConfig *dnsconfig = new runtime::DNSConfig();

        if ((networkType & NetworkType::Enum::Open) != NetworkType::Enum::Open)
        {
            dnsServers.push_back(ClearContainerHelper::ClearContainerLogHelperConstants::DnsServiceIP4Address);
        }
        
        WriteInfo(
            TraceType_ContainerRuntimeHandler,
            "DnsService is enabled.");

        for (auto const & dnsServer : dnsServers)
        {
            WriteInfo(
                TraceType_ContainerRuntimeHandler,
                "{0} set as dns server inside the container.",
                dnsServer);

            dnsconfig->add_servers(dnsServer);
        }

        if(!dnssearchoptions.empty())
        {
            dnsconfig->add_searches(dnssearchoptions);
        }

        dnsconfig->add_options(ClearContainerHelper::ClearContainerLogHelperConstants::NDotsConfig);
        pconfig->set_allocated_dns_config(dnsconfig);
    }
    else
    {
        // If the dns service is not enabled, we just replicate the host dns settings
        // inside the container.  
        struct __res_state res;
        if (0 == res_ninit(&res))
        {
            WriteInfo(
                TraceType_ContainerRuntimeHandler,  
                "DnsService is not enabled, replicating host dns settings inside the container.");
            runtime::DNSConfig *dnsconfig = new runtime::DNSConfig();
            for (int i = 0; i < res.nscount; i++)
            {
                char str[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &(res.nsaddr_list[i].sin_addr), str, INET_ADDRSTRLEN))
                {
                    dnsconfig->add_servers(str);
                }
            }

            for (int i = 0; res.dnsrch[i]; i++)
            {
                dnsconfig->add_searches(res.dnsrch[i]);
            }

            res_nclose(&res);
            pconfig->set_allocated_dns_config(dnsconfig);
        } 
    }

    pconfig->clear_port_mappings();
    for (auto pm : portmappings)
    {
        runtime::PortMapping *portmapping = pconfig->add_port_mappings();
        portmapping->set_host_ip(std::get<0>(pm));
        portmapping->set_protocol(std::get<1>(pm)==0 ? runtime::Protocol::TCP : runtime::Protocol::UDP);
        portmapping->set_container_port(std::get<2>(pm));
        portmapping->set_host_port(std::get<3>(pm));
    }

    pconfig->clear_labels();
    pconfig->clear_annotations();
    LinuxPodSandboxConfig *plinux = new LinuxPodSandboxConfig();
    {
        if (!cgparent.empty())
        {
            plinux->set_cgroup_parent(cgparent);
        }
        plinux->clear_security_context();
        plinux->clear_sysctls();
    }
    pconfig->set_allocated_linux(plinux);

    return pconfig;
}

void ContainerRuntimeHandler::printPodSandboxConfig(runtime::PodSandboxConfig* podSandboxConfig)
{
    if (podSandboxConfig->has_metadata())
    {
        runtime::PodSandboxMetadata pmeta = podSandboxConfig->metadata();
        WriteNoise(TraceType_ContainerRuntimeHandler,
            "Pod sandbox metadata name:{0}, uid:{1}, namespace:{2}, attempt:{3}",
            pmeta.name(),
            pmeta.uid(),
            pmeta.namespace_(),
            pmeta.attempt());
    }

    WriteNoise(TraceType_ContainerRuntimeHandler, "Pod sandbox host name:{0}", podSandboxConfig->hostname());
    WriteNoise(TraceType_ContainerRuntimeHandler, "Pod sandbox log directory:{0}", podSandboxConfig->log_directory());

    if (podSandboxConfig->has_dns_config())
    {
        runtime::DNSConfig dnsConfig = podSandboxConfig->dns_config();
        
        if (dnsConfig.servers_size() > 0)
        {
            wstring servers;
            for (auto const & s : dnsConfig.servers())
            {
                if (servers.empty())
                {
                    servers = wformatString("{0}", s);
                }
                else
                {
                    servers = wformatString("{0},{1}", servers, s);
                }
            }

            WriteNoise(TraceType_ContainerRuntimeHandler,
                "Pod sandbox dns config servers:{0}",
                servers);
        }

        if (dnsConfig.searches_size() > 0)
        {
            wstring searches;
            for (auto const & s : dnsConfig.searches())
            {
                if (searches.empty())
                {
                    searches = wformatString("{0}", s);
                }
                else
                {
                    searches = wformatString("{0},{1}", searches, s);
                }
            }

            WriteNoise(TraceType_ContainerRuntimeHandler,
                "Pod sandbox dns config searches:{0}",
                searches);
        }
    }

    if (podSandboxConfig->port_mappings_size() > 0)
    {
        for (auto const & pm : podSandboxConfig->port_mappings())
        {
            WriteNoise(TraceType_ContainerRuntimeHandler,
                "Pod sandbox port mapping host ip:{0}, protocol:{1}, container port:{2}, host port:{3}",
                pm.host_ip(),
                pm.protocol() == runtime::Protocol::TCP ? "TCP" : "UDP",
                pm.container_port(),
                pm.host_port());
        }
    }

    if (podSandboxConfig->has_linux())
    {
        runtime::LinuxPodSandboxConfig linux = podSandboxConfig->linux();
        WriteNoise(TraceType_ContainerRuntimeHandler,
            "Pod sandbox linux cgroup parent:{0}",
             linux.cgroup_parent());
    }
}

class ContainerRuntimeHandler::InitializeAsyncOperation : public AsyncOperation, private TextTraceComponent<TraceTaskCodes::Hosting>
{
public:
    friend class ContainerRuntimeHandler;

    InitializeAsyncOperation(
            wstring const & traceid,
            TimeSpan const timeout,
            AsyncCallback const &callback,
            AsyncOperationSPtr const &parent)
            : AsyncOperation(callback, parent),
              callback_(callback),
              ownerTraceId_(traceid)
    {
        timeout_ = static_cast<ULONG>(timeout.TotalMilliseconds());
    }

    virtual ~InitializeAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation) {
        auto thisPtr = AsyncOperation::End<InitializeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        {
            AcquireWriteLock lock(InitializationLock);
            if (InitState == InitializationState::Initialized)
            {
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
                return;
            }
            else if (InitState == InitializationState::Initializing)
            {
                PendingOperations->push_back(thisSPtr);
                return;
            }
            InitState = InitializationState::Initializing;
            PendingOperations->push_back(thisSPtr);
        }
        // start initialization thread
        pthread_attr_t pthreadAttr;
        Invariant(pthread_attr_init(&pthreadAttr) == 0);
        Invariant(pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED) == 0);

        auto retval = pthread_create(&tid_, &pthreadAttr, &ContainerRuntimeHandler::InitializationThread, this);
        Invariant(retval == 0);

        pthread_attr_destroy(&pthreadAttr);
    }

    virtual void OnCompleted()
    {
        __super::OnCompleted();
    }

private:
    Common::AsyncCallback callback_;
    ULONG timeout_;
    wstring ownerTraceId_;
    pthread_t tid_;
};

class ContainerRuntimeHandler::CrioInvokeAsyncOperation : public Common::AsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    CrioInvokeAsyncOperation(
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent),
              callback_(callback),
              ownerTraceId_(traceid),
              timeoutHelper_(timeout)
    {
        timeout_ = static_cast<ULONG>(timeout.TotalMilliseconds());
    }

    virtual ~CrioInvokeAsyncOperation()
    {
    }

    static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CrioInvokeAsyncOperation>(operation)->Error;
    }

protected:
    virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = BeginInitialize(
                ownerTraceId_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const& operation)
                {
                    this->EnsureCrioServiceCompleted(operation, false);
                },
                thisSPtr);
        EnsureCrioServiceCompleted(operation, true);
    }

    void EnsureCrioServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSyncronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSyncronously)
        {
            return;
        }
        auto error = EndInitialize(operation);
        if (!error.IsSuccess())
        {
            WriteInfo(TraceType_ContainerRuntimeHandler, ownerTraceId_, "End(Initialization): ErrorCode={0}", error);
            TryComplete(operation->Parent, error);
            return;
        }
        OnOperation(operation->Parent);
    }

    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
    }

    virtual void OnCompleted()
    {
        __super::OnCompleted();
    }

private:
    Common::AsyncCallback callback_;
    ULONG timeout_;
    TimeoutHelper timeoutHelper_;
    wstring ownerTraceId_;
};

class ContainerRuntimeHandler::CrioAsyncClientCallStruct
{
public:
    grpc::ClientContext context;
    grpc::Status status;

public:
    virtual ~CrioAsyncClientCallStruct() { }
    virtual void ProcessResult() { }
};

void* ContainerRuntimeHandler::CompletionQueueThread(void *arg)
{
    void* tag;
    bool ok = true;

    WriteInfo(TraceType_ContainerRuntimeHandler, "Entering CompletionQueueThread");

    CompletionQueueThreadStatus.store(UtilityThreadState::ThreadRunning);

    while (CompletionQueueThreadStatus.load() != UtilityThreadState::ThreadCancelling)
    {
        auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(1);
        auto ret = ContainerRuntimeHandler::GetSingleton()->compQueue_.AsyncNext(&tag, &ok, deadline);
        if (ret == grpc::CompletionQueue::TIMEOUT) continue;
        if (ok)
        {
            CrioAsyncClientCallStruct *call = (CrioAsyncClientCallStruct *)tag;
            Threadpool::Post(
                [call] { call->ProcessResult(); delete call; }
            );
        }
    }

    CompletionQueueThreadStatus.store(UtilityThreadState::ThreadNone);
    WriteInfo(TraceType_ContainerRuntimeHandler, "Exiting CompletionQueueThread");
    return 0;
}

class ContainerRuntimeHandler::CrioListPodSandboxAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class ListPodSandboxClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::ListPodSandboxResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::ListPodSandboxResponse>> respReader;
        shared_ptr<CrioListPodSandboxAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (!status.ok())
            {
                error = ErrorCodeValue::OperationFailed;
                WriteInfo(TraceType_ContainerRuntimeHandler, "ListPodSandbox gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
            }

            int nItems = reply.items_size();
            if (nItems)
            {
                op->items_.resize(nItems);
                for (int i = 0; i < nItems; i++)
                {
                    // id, metadata, state, created_at, labels, annotations
                    op->items_[i] = reply.items(i);
                }
            }
            op->TryComplete(op, error);
        }

        virtual ~ListPodSandboxClientCallStruct()
        {
        }
    };

public:
    CrioListPodSandboxAsyncOperation(
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent)
    {
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::ListPodSandboxRequest request;
        grpc::ClientContext context;

        request.clear_filter();

        ListPodSandboxClientCallStruct *call = new ListPodSandboxClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncListPodSandbox(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioListPodSandboxAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated ListPodSandbox with AsyncClientCallStruct");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, vector<runtime::PodSandbox> & items)
    {
        auto thisPtr = AsyncOperation::End<CrioListPodSandboxAsyncOperation>(operation);
        items = thisPtr->items_;
        return thisPtr->Error;
    }

private:
    vector<runtime::PodSandbox> items_;
};

class ContainerRuntimeHandler::CrioRunPodSandboxAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class RunPodSandboxClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::RunPodSandboxResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::RunPodSandboxResponse>> respReader;
        shared_ptr<CrioRunPodSandboxAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (status.ok())
            {
                op->podSandboxId_ = reply.pod_sandbox_id();
                PutPodSandboxInfo(op->podName_, op->podSandboxId_, op->appHostId_, op->nodeId_, op->logdirectory_);
                WriteInfo(TraceType_ContainerRuntimeHandler, "Finished RunPodSandbox: apphostid: {0}, nodeid: {1}, podname: {2}",
                          op->appHostId_, op->nodeId_, op->podName_);

            }
            else
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "RunPodSandbox gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
                error = ErrorCodeValue::OperationFailed;
            }
            op->TryComplete(op, error);
        }

        virtual ~RunPodSandboxClientCallStruct()
        {
        }
    };

public:
    CrioRunPodSandboxAsyncOperation(
            string const & apphostid,
            string const & nodeid,
            string const & name,
            string const & cgparent,
            string const & hostname,
            string const & logdirectory,
            ServiceModel::NetworkType::Enum networkType,
            bool isdnsserviceenabled,
            string const & dnssearchoptions,
            vector<string> const & dnsServers,
            vector<tuple<string,int,int,int>> portmappings,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              appHostId_(apphostid), nodeId_(nodeid), podName_(name), cgParent_(cgparent),
              hostname_(hostname), logdirectory_(logdirectory), networkType_(networkType),
              isdnsserviceenabled_(isdnsserviceenabled), dnssearchoptions_(dnssearchoptions), 
              dnsServers_(dnsServers), portMappings_(portmappings)
    {
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::RunPodSandboxRequest request;
        grpc::ClientContext context;

        runtime::PodSandboxConfig *pconfig = genPodSandboxConfig(podName_, cgParent_, hostname_, logdirectory_, networkType_, isdnsserviceenabled_, dnssearchoptions_, dnsServers_, portMappings_);
        request.set_allocated_config(pconfig);
        printPodSandboxConfig(pconfig);

        RunPodSandboxClientCallStruct *call = new RunPodSandboxClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncRunPodSandbox(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioRunPodSandboxAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated RunPodSandbox with AsyncClientCallStruct: apphostid: {0}, nodeid: {1}, podname: {2}",
                  appHostId_, nodeId_, podName_);
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & podSandboxId)
    {
        auto thisPtr = AsyncOperation::End<CrioRunPodSandboxAsyncOperation>(operation);
        podSandboxId = Common::StringUtility::Utf8ToUtf16(thisPtr->podSandboxId_);
        return thisPtr->Error;
    }

private:
    string appHostId_;
    string nodeId_;
    string podName_;
    string cgParent_;
    string hostname_;
    string logdirectory_;
    ServiceModel::NetworkType::Enum networkType_;
    bool isdnsserviceenabled_;
    string dnssearchoptions_;
    vector<string> dnsServers_;
    vector<tuple<string,int,int,int>> portMappings_;
    string podSandboxId_;
};


class ContainerRuntimeHandler::CrioStopPodSandboxAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class StopPodSandboxClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::StopPodSandboxResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::StopPodSandboxResponse>> respReader;
        shared_ptr<CrioStopPodSandboxAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (!status.ok())
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "StopPodSandbox gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
                error = ErrorCodeValue::OperationFailed;
            }
            op->TryComplete(op, error);
        }

        virtual ~StopPodSandboxClientCallStruct()
        {
        }
    };

public:
    CrioStopPodSandboxAsyncOperation(
            string const & podname,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              podName_(podname)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CrioStopPodSandboxAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::StopPodSandboxRequest request;
        grpc::ClientContext context;

        if (!GetPodIdFromName(podName_, sandboxId_))
        {
            error = ErrorCodeValue::InvalidArgument;
            thisSPtr->TryComplete(thisSPtr, error);
            return;
        }

        request.set_pod_sandbox_id(sandboxId_);

        StopPodSandboxClientCallStruct *call = new StopPodSandboxClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncStopPodSandbox(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioStopPodSandboxAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated StopPodSandbox with AsyncClientCallStruct: Name: {0}. Id: {1}", podName_, sandboxId_);
    }

private:
    string podName_;
    string sandboxId_;
};

class ContainerRuntimeHandler::CrioRemovePodSandboxAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class RemovePodSandboxClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::RemovePodSandboxResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::RemovePodSandboxResponse>> respReader;
        shared_ptr<CrioRemovePodSandboxAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (status.ok())
            {
                DelPodSandboxInfo(op->podName_, op->sandboxId_);
            }
            else
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "RemovePodSandbox gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
                error = ErrorCodeValue::OperationFailed;
            }
            op->TryComplete(op, error);
        }

        virtual ~RemovePodSandboxClientCallStruct()
        {
        }
    };

public:
    CrioRemovePodSandboxAsyncOperation(
            string const & podname,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              podName_(podname)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CrioRemovePodSandboxAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::RemovePodSandboxRequest request;
        grpc::ClientContext context;

        if (!GetPodIdFromName(podName_, sandboxId_))
        {
            error = ErrorCodeValue::InvalidArgument;
            thisSPtr->TryComplete(thisSPtr, error);
            return;
        }

        request.set_pod_sandbox_id(sandboxId_);

        RemovePodSandboxClientCallStruct *call = new RemovePodSandboxClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncRemovePodSandbox(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioRemovePodSandboxAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated RemovePodSandbox with AsyncClientCallStruct: Name: {0}. Id: {1}", podName_, sandboxId_);
    }

private:
    string podName_;
    string sandboxId_;
};

class ContainerRuntimeHandler::CrioPullImageAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class PullImageClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::PullImageResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::PullImageResponse>> respReader;
        shared_ptr<CrioPullImageAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (!status.ok())
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "PullImage gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
                error = status.error_message().find("Error committing the finished image") != string::npos ?
                        ErrorCodeValue::AlreadyExists : ErrorCodeValue::OperationFailed;
            }
            op->TryComplete(op, error);
        }

        virtual ~PullImageClientCallStruct()
        {
        }
    };

public:
    CrioPullImageAsyncOperation(
            string const & imgname,
            string const & authusr,
            string const & authpass,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              imgName_(imgname), authUser_(authusr), authPass_(authpass)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CrioPullImageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::PullImageRequest request;
        grpc::ClientContext context;

        ImageSpec *pimgspec = new ImageSpec();
        pimgspec->set_image(imgName_);
        request.set_allocated_image(pimgspec);

        if (authUser_.empty())
        {
            request.clear_auth();
        }
        else
        {
            AuthConfig *pauthcfg = new AuthConfig();
            pauthcfg->set_username(authUser_);
            pauthcfg->set_password(authPass_);
            request.set_allocated_auth(pauthcfg);
        }

        PullImageClientCallStruct *call = new PullImageClientCallStruct();
        call->respReader = handler->imgStub_->PrepareAsyncPullImage(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioPullImageAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated PullImage with AsyncClientCallStruct: Image: {0}", imgName_);
    }

private:
    string imgName_;
    string authUser_;
    string authPass_;
};

class ContainerRuntimeHandler::CrioListContainersAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class ListContainersClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::ListContainersResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::ListContainersResponse>> respReader;
        shared_ptr<CrioListContainersAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (!status.ok())
            {
                error = ErrorCodeValue::OperationFailed;;
                WriteInfo(TraceType_ContainerRuntimeHandler, "ListContainers gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
            }
            int nItems = reply.containers_size();
            if (nItems)
            {
                op->items_.resize(nItems);
                for (int i = 0; i < nItems; i++)
                {
                    // id, pod_sandbox_id, metadata, image, image_ref, state, created_at, labels, annotations
                    op->items_[i] = reply.containers(i);
                }
            }
            op->TryComplete(op, error);
        }

        virtual ~ListContainersClientCallStruct()
        {
        }
    };

public:
    CrioListContainersAsyncOperation(
            map<string, string> labelSelectorFilter,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              labelSelectorFilter_(labelSelectorFilter)
    {
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::ListContainersRequest request;
        grpc::ClientContext context;

        auto labelsSelector = request.mutable_filter()->mutable_label_selector();

        for (auto const &entry : labelSelectorFilter_)
        {
            WriteInfo(
                TraceType_ContainerRuntimeHandler,
                "ContainerRuntimeHandler::CrioListContainersAsyncOperation. labelselectorFilter.key={0} value={1}",
                entry.first, entry.second);
            (*labelsSelector)[entry.first] = entry.second;
        }

        ListContainersClientCallStruct *call = new ListContainersClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncListContainers(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioListContainersAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated ListContainers with AsyncClientCallStruct");
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, vector<runtime::Container> & items)
    {
        auto thisPtr = AsyncOperation::End<CrioListContainersAsyncOperation>(operation);
        items = thisPtr->items_;
        return thisPtr->Error;
    }

private:
    map<string, string> labelSelectorFilter_;
    vector<runtime::Container> items_;
};

class ContainerRuntimeHandler::CrioCreateContainerAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class CreateContainerClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::CreateContainerResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::CreateContainerResponse>> respReader;
        shared_ptr<CrioCreateContainerAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (status.ok())
            {
                op->containerId_ = reply.container_id();
                PutContainerInfo(op->containerName_, op->containerId_, op->appHostId_, op->nodeId_, op->logpath_);
                WriteInfo(TraceType_ContainerRuntimeHandler, "Finished CreateContainer: apphostid: {0}, nodeid: {1}, container name: {2}",
                          op->appHostId_, op->nodeId_, op->containerName_);
            }
            else
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "CreateContainer gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
                error = status.error_message().find("image not known") != string::npos ?
                        ErrorCodeValue::NotFound :
                        ErrorCodeValue::OperationFailed;
            }
            op->TryComplete(op, error);
        }

        virtual ~CreateContainerClientCallStruct()
        {
        }
    };

public:
    CrioCreateContainerAsyncOperation(
            string const & apphostid,
            string const & nodeid,
            string const & podname,
            string const & containername,
            string const & imgname,
            vector<string> const &cmds,
            vector<string> const & args,
            vector<pair<string, string>> const & envs,
            vector<tuple<string, string, bool>> const & mounts,
            vector<pair<string, string>> const & labels,
            string const & workdir,
            string const & logpath,
            bool interactive,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              appHostId_(apphostid), nodeId_(nodeid), podName_(podname), containerName_(containername),
              imgName_(imgname), cmds_(cmds), args_(args), envs_(envs), mounts_(mounts),
              labels_(labels), workdir_(workdir), logpath_(logpath), interactive_(interactive)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & sandboxId, __out wstring & containerId)
    {
        auto thisPtr = AsyncOperation::End<CrioCreateContainerAsyncOperation>(operation);
        sandboxId = Common::StringUtility::Utf8ToUtf16(thisPtr->sandboxId_);
        containerId = Common::StringUtility::Utf8ToUtf16(thisPtr->containerId_);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::CreateContainerRequest request;
        grpc::ClientContext context;

        if (!GetPodIdFromName(podName_, sandboxId_))
        {
            error = ErrorCodeValue::InvalidArgument;
            thisSPtr->TryComplete(thisSPtr, error);
            return;
        }

        runtime::ContainerConfig *pconfig = genContainerConfig(containerName_, imgName_, cmds_, args_, envs_, mounts_, labels_, workdir_, logpath_, interactive_);
        request.set_allocated_config(pconfig);
        request.set_pod_sandbox_id(sandboxId_);
        CreateContainerClientCallStruct *call = new CreateContainerClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncCreateContainer(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioCreateContainerAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler,
                  "Initiated CreateContainer with AsyncClientCallStruct: apphostid: {0}, nodeid: {1}, Name: {2}. PodName: {3}. PodId: {4}. Img: {5}. Cmd: {6}",
                  appHostId_, nodeId_, containerName_, podName_, sandboxId_, imgName_, cmds_.size() ? cmds_[0] : "NONE");
    }

private:
    string appHostId_;
    string nodeId_;
    string podName_;
    string sandboxId_;
    string containerName_;
    string imgName_;
    vector<string> cmds_;
    vector<string> args_;
    vector<pair<string, string>> envs_;
    vector<tuple<string, string, bool>> mounts_;
    vector<pair<string, string>> labels_;
    string workdir_;
    string logpath_;
    bool interactive_;

    string containerId_;
};

class ContainerRuntimeHandler::CrioStartContainerAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class StartContainerClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::StartContainerResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::StartContainerResponse>> respReader;
        shared_ptr<CrioStartContainerAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (!status.ok())
            {
                error = ErrorCodeValue::OperationFailed;
                WriteInfo(TraceType_ContainerRuntimeHandler, "StartContainer gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
            }
            op->TryComplete(op, error);
        }

        virtual ~StartContainerClientCallStruct()
        {
        }
    };

public:
    CrioStartContainerAsyncOperation(
            string const & containername,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              containerName_(containername)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CrioStartContainerAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::StartContainerRequest request;
        grpc::ClientContext context;

        if (!GetContainerIdFromName(containerName_, containerId_))
        {
            error = ErrorCodeValue::InvalidArgument;
            thisSPtr->TryComplete(thisSPtr, error);
            return;
        }

        request.set_container_id(containerId_);

        StartContainerClientCallStruct *call = new StartContainerClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncStartContainer(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioStartContainerAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated StartContainer with AsyncClientCallStruct: Name: {0}. Id: {1}",
                  containerName_, containerId_);
    }

private:
    string containerName_;
    string containerId_;
};

class ContainerRuntimeHandler::CrioStopContainerAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class StopContainerClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::StopContainerResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::StopContainerResponse>> respReader;
        shared_ptr<CrioStopContainerAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (!status.ok())
            {
                error = ErrorCodeValue::OperationFailed;
                WriteInfo(TraceType_ContainerRuntimeHandler, "StopContainer gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
            }
            op->TryComplete(op, error);
        }

        virtual ~StopContainerClientCallStruct()
        {
        }
    };

public:
    CrioStopContainerAsyncOperation(
            string const & containername,
            int64_t timeout,
            wstring const & traceid,
            Common::TimeSpan optimeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, optimeout, callback, parent),
              containerName_(containername), timeout_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CrioStopContainerAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::StopContainerRequest request;
        grpc::ClientContext context;

        if (!GetContainerIdFromName(containerName_, containerId_))
        {
            error = ErrorCodeValue::InvalidArgument;
            thisSPtr->TryComplete(thisSPtr, error);
            return;
        }

        request.set_container_id(containerId_);
        request.set_timeout(timeout_);

        StopContainerClientCallStruct *call = new StopContainerClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncStopContainer(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioStopContainerAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated StopContainer with AsyncClientCallStruct: Name: {0}. Id: {1}",
                  containerName_, containerId_);
    }

private:
    string containerName_;
    string containerId_;
    int64_t timeout_;
};

class ContainerRuntimeHandler::CrioRemoveContainerAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

    class RemoveContainerClientCallStruct : public CrioAsyncClientCallStruct
    {
    public:
        runtime::RemoveContainerResponse reply;
        std::unique_ptr<grpc::ClientAsyncResponseReader<runtime::RemoveContainerResponse>> respReader;
        shared_ptr<CrioRemoveContainerAsyncOperation> op;

    public:
        virtual void ProcessResult()
        {
            Common::ErrorCode error;
            if (status.ok())
            {
                DelContainerInfo(op->containerName_, op->containerId_);
            }
            else
            {
                WriteInfo(TraceType_ContainerRuntimeHandler, "RemoveContainer gRPC-error: {0}: {1}", (int)status.error_code(), status.error_message());
                error = ErrorCodeValue::OperationFailed;
            }
            op->TryComplete(op, error);
        }

        virtual ~RemoveContainerClientCallStruct()
        {
        }
    };

public:
    CrioRemoveContainerAsyncOperation(
            string const & containername,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              containerName_(containername)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CrioRemoveContainerAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Common::ErrorCode error;
        ContainerRuntimeHandler *handler = ContainerRuntimeHandler::GetSingleton();
        runtime::RemoveContainerRequest request;
        grpc::ClientContext context;

        if (!GetContainerIdFromName(containerName_, containerId_))
        {
            error = ErrorCodeValue::InvalidArgument;
            thisSPtr->TryComplete(thisSPtr, error);
            return;
        }

        request.set_container_id(containerId_);

        RemoveContainerClientCallStruct *call = new RemoveContainerClientCallStruct();
        call->respReader = handler->rtStub_->PrepareAsyncRemoveContainer(&call->context, request, &handler->compQueue_);
        call->op = std::dynamic_pointer_cast<CrioRemoveContainerAsyncOperation>(shared_from_this());
        call->respReader->StartCall();
        call->respReader->Finish(&call->reply, &call->status, (void*)call);
        WriteInfo(TraceType_ContainerRuntimeHandler, "Initiated RemoveContainer with AsyncClientCallStruct: Name: {0}. Id: {1}",
                  containerName_, containerId_);
    }

private:
    string containerName_;
    string containerId_;
};


class ContainerRuntimeHandler::CrioGetContainerLogAsyncOperation : public CrioInvokeAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

public:
    CrioGetContainerLogAsyncOperation(
            string const & containername,
            string const & tailarg,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioInvokeAsyncOperation(traceid, timeout, callback, parent),
              containerName_(containername),
              tailArg_(tailarg)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, wstring & logContent)
    {
        auto thisPtr = AsyncOperation::End<CrioGetContainerLogAsyncOperation>(operation);
        Common::StringUtility::Utf8ToUtf16(thisPtr->fileContent_, logContent);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Threadpool::Post(
        [this, thisSPtr]
        {
            ErrorCode error;
            if (!GetContainerIdFromName(containerName_, containerId_)
                || !GetContainerLogPath(containerName_, filePath_))
            {
                error = ErrorCodeValue::InvalidArgument;
                thisSPtr->TryComplete(thisSPtr, error);
                return;
            }
            error = ReadLogsFromFilePath(this->filePath_, this->fileContent_);
            thisSPtr->TryComplete(thisSPtr, error);
        },
        TimeSpan::FromMilliseconds(Random().Next(5000)));
    }

    virtual ErrorCode ReadLogsFromFilePath(const string & filePath, string & fileContent)
    {
        ErrorCode error;
        struct stat st;
            if (stat(filePath.c_str(), &st))
            {
                error = ErrorCodeValue::FileNotFound;
                return error;
            }
            int fd = -1;
            char *p = 0;
            int sz = (int)st.st_size;
            do
            {
                if (-1 == (fd = open(filePath.c_str(), O_RDONLY))) break;
                if (MAP_FAILED == (p = (char*)mmap(0, sz, PROT_READ, MAP_SHARED, fd, 0))) break;
            } while(0);

            int start = sz - 1;
            string deftail = Common::StringUtility::Utf16ToUtf8(Constants::DefaultContainerLogsTail);
            int deftailnum = atoi(deftail.c_str());
            int tailnum = (tailArg_.empty() ? atoi(deftail.c_str()) : atoi(tailArg_.c_str()));
            if (tailnum <= 0) tailnum = deftailnum;
            for (; tailnum > 0 && start >= 0; --start)
            {
                if (p[start] == '\n' && start != sz - 1) --tailnum;
            }
            start++;
            fileContent = string(p + start, sz - start);

            if (p != MAP_FAILED) munmap(p, sz);
            if (fd != -1) close(fd);
        return error;
    }


private:
    string filePath_;
    string fileContent_;
    string containerName_;
    string tailArg_;
    string containerId_;
};

class ContainerRuntimeHandler::CrioGetContainerLogFromFilePathAsyncOperation : public CrioGetContainerLogAsyncOperation
{
public:
    friend class ContainerRuntimeHandler;

public:
    CrioGetContainerLogFromFilePathAsyncOperation(
            string const & filePath,
            string const & tailarg,
            wstring const & traceid,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : CrioGetContainerLogAsyncOperation("", tailarg, traceid, timeout, callback, parent),
              filePath_(filePath)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, wstring & logContent)
    {
        auto thisPtr = AsyncOperation::End<CrioGetContainerLogFromFilePathAsyncOperation>(operation);
        Common::StringUtility::Utf8ToUtf16(thisPtr->fileContent_, logContent);
        return thisPtr->Error;
    }

protected:
    virtual void OnOperation(Common::AsyncOperationSPtr const & thisSPtr)
    {
        Threadpool::Post(
        [this, thisSPtr]
        {
            ErrorCode error;
            error = ReadLogsFromFilePath(this->filePath_, this->fileContent_);
            thisSPtr->TryComplete(thisSPtr, error);
        },
        TimeSpan::FromMilliseconds(Random().Next(5000)));
    }
private:
    string filePath_;
    string fileContent_;
};


// ********************************************************************************************************************
// ContainerRuntimeHandler main APIs Implementation
//

ContainerRuntimeHandler::ContainerRuntimeHandler(std::shared_ptr<grpc::Channel> channel)
        : rpcChannel_ (channel),
          rtStub_ (runtime::RuntimeService::NewStub(channel)),
          imgStub_ (runtime::ImageService::NewStub(channel))
{
}

ContainerRuntimeHandler::~ContainerRuntimeHandler()
{
    compQueue_.Shutdown();
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginListPodSandbox(
        std::wstring traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CrioListPodSandboxAsyncOperation>(
            traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndListPodSandbox(
        __in Common::AsyncOperationSPtr const & operation,
        __out vector<runtime::PodSandbox> & items)
{
    return CrioListPodSandboxAsyncOperation::End(operation, items);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginRunPodSandbox(
        std::wstring const & apphostid,
        std::wstring const & nodeid,
        std::wstring const & podname,
        std::wstring const & cgparent,
        std::wstring const & hostname,
        std::wstring const & logdirectory,
        ServiceModel::NetworkType::Enum networkType,
        bool isdnsserviceenabled,
        std::wstring const & dnssearchoptions,
        std::vector<std::wstring> const & dnsServers,
        std::vector<std::tuple<std::wstring,int,int,int>> portmappings,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string apphostidA = Common::StringUtility::Utf16ToUtf8(apphostid);
    std::string nodeidA = Common::StringUtility::Utf16ToUtf8(nodeid);
    std::string podnameA = Common::StringUtility::Utf16ToUtf8(podname);
    std::string cgparentA = Common::StringUtility::Utf16ToUtf8(cgparent);
    std::string hostnameA = Common::StringUtility::Utf16ToUtf8(hostname);
    std::string logdirectoryA = Common::StringUtility::Utf16ToUtf8(logdirectory);
    std::string dnssearchoptionsA = Common::StringUtility::Utf16ToUtf8(dnssearchoptions);

    std::vector<std::string> dnsServersA;
    for (auto const & dnsServer : dnsServers)
    {
        dnsServersA.push_back(Common::StringUtility::Utf16ToUtf8(dnsServer));
    }

    std::vector<std::tuple<std::string,int,int,int>> portmappingsA;
    for (auto pm : portmappings)
    {
        portmappingsA.push_back(make_tuple(Common::StringUtility::Utf16ToUtf8(std::get<0>(pm)),
                                           std::get<1>(pm),std::get<2>(pm), std::get<3>(pm)));
    }

    return AsyncOperation::CreateAndStart<CrioRunPodSandboxAsyncOperation>(
            apphostidA, nodeidA, podnameA, cgparentA, hostnameA, logdirectoryA, networkType, isdnsserviceenabled, dnssearchoptionsA, dnsServersA, portmappingsA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndRunPodSandbox(
        __in Common::AsyncOperationSPtr const & operation)
{
    std::wstring sandboxId;
    return CrioRunPodSandboxAsyncOperation::End(operation, sandboxId);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginStopPodSandbox(
        std::wstring const & podname,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string podnameA = Common::StringUtility::Utf16ToUtf8(podname);
    return AsyncOperation::CreateAndStart<CrioStopPodSandboxAsyncOperation>(
            podnameA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndStopPodSandbox(
        __in Common::AsyncOperationSPtr const & operation)
{
    return CrioStopContainerAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginRemovePodSandbox(
        std::wstring const & podname,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string podnameA = Common::StringUtility::Utf16ToUtf8(podname);
    return AsyncOperation::CreateAndStart<CrioRemovePodSandboxAsyncOperation>(podnameA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndRemovePodSandbox(
        __in Common::AsyncOperationSPtr const & operation)
{
    return CrioRemoveContainerAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginPullImage(
        std::wstring const & imgname,
        std::wstring const & authusr,
        std::wstring const & authpass,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string imgnameA = Common::StringUtility::Utf16ToUtf8(imgname);
    std::string authusrA = Common::StringUtility::Utf16ToUtf8(authusr);
    std::string authpassA = Common::StringUtility::Utf16ToUtf8(authpass);
    return AsyncOperation::CreateAndStart<CrioPullImageAsyncOperation>(imgnameA, authusrA, authpassA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndPullImage(
        __in Common::AsyncOperationSPtr const & operation)
{
    return CrioPullImageAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginListContainers(
        std::map<std::wstring, std::wstring> labelSelectorFilter,
        std::wstring traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    WriteInfo(
        TraceType_ContainerRuntimeHandler,
        "ContainerRuntimeHandler::BeginListContainers. labelselectorFilter.size={0}",
        labelSelectorFilter.size());

    std:map<std::string, std::string> labelSelectorFilterA;

    for (auto const &entry : labelSelectorFilter)
    {
        WriteInfo(
            TraceType_ContainerRuntimeHandler,
            "ContainerRuntimeHandler::BeginListContainers. labelselectorFilter.key={0} value={1}",
            entry.first, entry.second);

        labelSelectorFilterA.insert(make_pair(
            Common::StringUtility::Utf16ToUtf8(entry.first),
            Common::StringUtility::Utf16ToUtf8(entry.second)
        ));
    }

    return AsyncOperation::CreateAndStart<CrioListContainersAsyncOperation>(
            labelSelectorFilterA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndListContainers(
        __in Common::AsyncOperationSPtr const & operation,
        __out vector<runtime::Container> & items)
{
    return CrioListContainersAsyncOperation::End(operation, items);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginCreateContainer(
        std::wstring const & apphostid,
        std::wstring const & nodeid,
        std::wstring const & podname,
        std::wstring const & containername,
        std::wstring const & imgname,
        vector<wstring> const &cmds,
        vector<wstring> const & args,
        vector<pair<wstring, wstring>> const & envs,
        vector<tuple<wstring, wstring, bool>> const & mounts,
        vector<pair<wstring, wstring>> const & labels,
        wstring const & workdir,
        wstring const & logpath,
        bool interactive,
        std::wstring traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string apphostidA = Common::StringUtility::Utf16ToUtf8(apphostid);
    std::string nodeidA = Common::StringUtility::Utf16ToUtf8(nodeid);
    std::string podnameA = Common::StringUtility::Utf16ToUtf8(podname);
    std::string containernameA = Common::StringUtility::Utf16ToUtf8(containername);
    std::string imgnameA = Common::StringUtility::Utf16ToUtf8(imgname);
    vector<string> cmdsA;
    for (auto cmd :cmds) cmdsA.push_back(Common::StringUtility::Utf16ToUtf8(cmd));
    vector<string> argsA;
    for (auto arg :args)
    {
        argsA.push_back(Common::StringUtility::Utf16ToUtf8(arg));
    }
    vector<pair<string, string>> envsA;
    for (auto env : envs)
    {
        envsA.push_back(make_pair(Common::StringUtility::Utf16ToUtf8(env.first),
                                  Common::StringUtility::Utf16ToUtf8(env.second)));
    }
    vector<tuple<string, string, bool>> mountsA;
    for (auto mntspec : mounts)
    {
        mountsA.push_back(make_tuple(Common::StringUtility::Utf16ToUtf8(std::get<0>(mntspec)),
                                     Common::StringUtility::Utf16ToUtf8(std::get<1>(mntspec)), std::get<2>(mntspec)));
    }
    vector<pair<string, string>> labelsA;
    for (auto label : labels)
    {
        labelsA.push_back(make_pair(Common::StringUtility::Utf16ToUtf8(label.first),
                                    Common::StringUtility::Utf16ToUtf8(label.second)));
    }

    string workdirA = Common::StringUtility::Utf16ToUtf8(workdir);
    string logpathA = Common::StringUtility::Utf16ToUtf8(logpath);

    AsyncOperationSPtr operation = std::make_shared<CrioCreateContainerAsyncOperation>(
            apphostidA, nodeidA, podnameA, containernameA, imgnameA, cmdsA, argsA, envsA, mountsA, labelsA, workdirA, logpathA, interactive,
            traceid, TimeSpan::MaxValue, callback, parent
    );
    operation->Start(operation);
    return operation;
}

Common::ErrorCode ContainerRuntimeHandler::EndCreateContainer(
        __in Common::AsyncOperationSPtr const & operation)
{
    std::wstring sandboxId;
    std::wstring containerId;
    return CrioCreateContainerAsyncOperation::End(operation, sandboxId, containerId);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginStartContainer(
        std::wstring const & containername,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string containernameA = Common::StringUtility::Utf16ToUtf8(containername);
    return AsyncOperation::CreateAndStart<CrioStartContainerAsyncOperation>(containernameA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndStartContainer(
        __in Common::AsyncOperationSPtr const & operation)
{
    return CrioStartContainerAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginStopContainer(
        std::wstring const & containername,
        int64_t timeout,
        std::wstring const & traceid,
        Common::TimeSpan const optimeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string containernameA = Common::StringUtility::Utf16ToUtf8(containername);
    return AsyncOperation::CreateAndStart<CrioStopContainerAsyncOperation>(containernameA, timeout, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndStopContainer(
        __in Common::AsyncOperationSPtr const & operation)
{
    return CrioStopContainerAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginRemoveContainer(
        std::wstring const & containername,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string containernameA = Common::StringUtility::Utf16ToUtf8(containername);
    return AsyncOperation::CreateAndStart<CrioRemoveContainerAsyncOperation>(containernameA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndRemoveContainer(
        __in Common::AsyncOperationSPtr const & operation)
{
    return CrioRemoveContainerAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginGetContainerLog(
        std::wstring const & containername,
        std::wstring const & tailarg,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string containernameA = Common::StringUtility::Utf16ToUtf8(containername);
    std::string tailargA = Common::StringUtility::Utf16ToUtf8(tailarg);
    return AsyncOperation::CreateAndStart<CrioGetContainerLogAsyncOperation>(containernameA, tailargA, traceid, TimeSpan::MaxValue, callback, parent);
}

 Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginGetContainerLogByPath(
        std::wstring const & containerLogPath,
        std::wstring const & tailarg,
        std::wstring const & traceid,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
{
    std::string containerLogPathA = Common::StringUtility::Utf16ToUtf8(containerLogPath);
    std::string tailargA = Common::StringUtility::Utf16ToUtf8(tailarg);

    return AsyncOperation::CreateAndStart<CrioGetContainerLogFromFilePathAsyncOperation>(containerLogPathA, tailargA, traceid, TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode ContainerRuntimeHandler::EndGetContainerLog(
        __in Common::AsyncOperationSPtr const & operation, wstring &fileContent)
{
    return CrioGetContainerLogAsyncOperation::End(operation, fileContent);
}

Common::ErrorCode ContainerRuntimeHandler::EndGetContainerLogByPath(
    __in Common::AsyncOperationSPtr const & operation, wstring &fileContent)
{
    return CrioGetContainerLogFromFilePathAsyncOperation::End(operation, fileContent);
}

AsyncOperationSPtr ContainerRuntimeHandler::BeginInitialize(
        std::wstring const & traceid,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<InitializeAsyncOperation>(traceid, timeout, callback, parent);
}

ErrorCode ContainerRuntimeHandler::EndInitialize(AsyncOperationSPtr const & operation)
{
    return InitializeAsyncOperation::End(operation);
}
