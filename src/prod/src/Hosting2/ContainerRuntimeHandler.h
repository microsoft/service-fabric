// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ContainerRuntimeHandler : public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public: // constructor, destructor
        ContainerRuntimeHandler(std::shared_ptr<grpc::Channel> channel);

        virtual ~ContainerRuntimeHandler();

    public: // the singleton
        static ContainerRuntimeHandler* GetSingleton();

        static bool GetPodIdFromName(string const & name, string &id);

    private:
        static ContainerRuntimeHandler *ContainerRuntimeHandlerSingleton;
        static Common::RwLock SingletonObjectLock;

    private: // initialization (crio daemon could crash)
        enum InitializationState
        {
            Uninitialized = 0,
            Initializing  = 1,
            Initialized   = 2
        };
        static Common::RwLock InitializationLock;
        static string ContainerLogRoot;
        static InitializationState InitState;
        static vector<AsyncOperationSPtr> *PendingOperations;

        static void CleanPendingOperations(ErrorCode const & error);

        static void* InitializationThread(void *arg);
        static bool InitializationThreadLoop(int iteration);
        static int InitializationThreadIteration;

        static void CrioProcessMonitor(pid_t pid, Common::ErrorCode const & waitResult, DWORD processExitCode);

        static pid_t ExecWithPrctl(string const & exePath, vector<string> const & args, vector<string> const & envs,
                                   ProcessWait::WaitCallback exitCallback);

    public: // container/pod status notification
        enum PodSandboxState
        {
            Ready    = 0,
            NotReady = 1
        };
        enum ContainerState
        {
            Created  = 0,
            Running  = 1,
            Exited   = 2,
            Unknown  = 3
        };

        static std::map<void*, void*> ContainerStatusCallbacks;
        static std::map<void*, void*> PodSandboxStatusCallbacks;

        typedef void (*ContainerStateUpdateCallback) (
                string const & apphostid, string const & nodeid, string const & name,
                ContainerState from, ContainerState to, void *arg);

        static void* RegisterContainerStateUpdateCallback(ContainerStateUpdateCallback func, void *arg);
        static void  UnregisterContainerStateUpdateCallback(void *func);

        typedef void (*PodSandboxStateUpdateCallback) (
                string const & apphostid, string const & nodeid, string const & name,
                PodSandboxState from, PodSandboxState to, void *arg);

        static void* RegisterPodSandboxStateUpdateCallback(PodSandboxStateUpdateCallback func, void *arg);
        static void  UnregisterPodSandboxStateUpdateCallback(void *func);

    private: // internal status of each pod/container, as well as name/id mapping
        struct PodSandboxInfo
        {
            string id_;
            string meta_name_;
            string meta_namespace_;
            int meta_attempt_;
            int64_t created_at_;
            PodSandboxState state_;
            string apphost_id_;
            string node_id_;
            string log_directory_;

            struct timespec create_timestamp_;

            PodSandboxInfo() : state_(PodSandboxState::NotReady){ };
            bool Update(const runtime::PodSandbox & pod);
        };

        struct ContainerInfo
        {
            string id_;
            string pod_sandbox_id_;
            string meta_name_;
            int meta_attempt_;
            int64_t created_at_;
            string image_ref_;
            ContainerState state_;
            string apphost_id_;
            string node_id_;
            string log_path_;

            struct timespec create_timestamp_;

            ContainerInfo() : state_(ContainerState::Created){ };
            void Update(const runtime::Container & ct);
        };

        static Common::RwLock CrioNameMapLock;
        static std::map<string, string> PodSandboxNameToIdMap;
        static std::map<string, string> ContainerNameToIdMap;
        static std::map<string, PodSandboxInfo> PodSandboxTbl;
        static std::map<string, ContainerInfo> ContainerTbl;

        static bool GetContainerIdFromName(string const & name, string &id);

        static void PutPodSandboxInfo(string const &name, string const &id, string const &apphostid, string const &nodeid, string const &logdirectory);

        static void DelPodSandboxInfo(string const &name, string const &id);

        static void PutContainerInfo(string const &name, string const &id, string const &apphostid, string const &nodeid, string const &logpath);

        static void DelContainerInfo(string const &name, string const &id);

        static bool GetContainerLogPath(string const &containername, string &path);

    private: // state polling thread, completion queue thread

        enum UtilityThreadState
        {
            ThreadNone       = 0,
            ThreadCreated    = 1,
            ThreadRunning    = 2,
            ThreadCancelling = 3
        };

        static std::atomic_int InitializationThreadStatus;

        static std::atomic_int StatePollThreadStatus;

        static std::atomic_int CompletionQueueThreadStatus;

        static void* StatePollThread(void *arg);

        static void* CompletionQueueThread(void *arg);

    public:
        static bool PodExists(wstring const & podName);

        static bool ContainerExists(wstring const & containerName);

    public: // async pod/container operations
        static Common::AsyncOperationSPtr BeginListPodSandbox(
                std::wstring traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndListPodSandbox(
                __in Common::AsyncOperationSPtr const & operation,
                __out vector<runtime::PodSandbox> & items);

        static Common::AsyncOperationSPtr BeginRunPodSandbox(
                std::wstring const & apphostid,
                std::wstring const & nodeid,
                std::wstring const & name,
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
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndRunPodSandbox(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginStopPodSandbox(
                std::wstring const & sandboxid,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode ContainerRuntimeHandler::EndStopPodSandbox(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginRemovePodSandbox(
                std::wstring const & sandboxid,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode ContainerRuntimeHandler::EndRemovePodSandbox(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginPullImage(
                std::wstring const & imgname,
                std::wstring const & authusr,
                std::wstring const & authpass,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode ContainerRuntimeHandler::EndPullImage(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr BeginListContainers(
                std::map<std::wstring, std::wstring> labelSelectorFilter,
                std::wstring traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndListContainers(
                __in Common::AsyncOperationSPtr const & operation,
                __out vector<runtime::Container> & items);

        static Common::AsyncOperationSPtr BeginCreateContainer(
                std::wstring const & apphostid,
                std::wstring const & nodeid,
                std::wstring const & sandboxid,
                std::wstring const & name,
                std::wstring const & imgname,
                vector<wstring> const &cmds,
                vector<wstring> const & args,
                vector<pair<wstring, wstring>> const & envs,
                vector<tuple<wstring, wstring, bool>> const & mounts,
                vector<pair<wstring, wstring>> const & labels,
                wstring const & workdir,
                wstring const & logpath,
                bool interactive,
                std::wstring const traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndCreateContainer(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginStartContainer(
                std::wstring const & containerid,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode ContainerRuntimeHandler::EndStartContainer(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginStopContainer(
                std::wstring const & containerid,
                int64_t timeout,
                std::wstring const & traceid,
                Common::TimeSpan const optimeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode ContainerRuntimeHandler::EndStopContainer(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr ContainerRuntimeHandler::BeginRemoveContainer(
                std::wstring const & containerid,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode ContainerRuntimeHandler::EndRemoveContainer(
                __in Common::AsyncOperationSPtr const & operation);

        static Common::AsyncOperationSPtr BeginGetContainerLog(
                std::wstring const & containername,
                std::wstring const & tailarg,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::AsyncOperationSPtr BeginGetContainerLogByPath(
                std::wstring const & containerLogPath,
                std::wstring const & tailarg,
                std::wstring const & traceid,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndGetContainerLog(
                __in Common::AsyncOperationSPtr const & operation, wstring &fileContent);

        static Common::ErrorCode EndGetContainerLogByPath(
                __in Common::AsyncOperationSPtr const & operation, wstring &fileContent);

        static Common::AsyncOperationSPtr BeginInitialize(
                std::wstring const & traceid,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndInitialize(AsyncOperationSPtr const & operation);

    private:
        class CrioAsyncClientCallStruct;
        class CrioInvokeAsyncOperation;
        class CrioListPodSandboxAsyncOperation;
        class CrioListContainersAsyncOperation;
        class CrioRunPodSandboxAsyncOperation;
        class CrioStopPodSandboxAsyncOperation;
        class CrioRemovePodSandboxAsyncOperation;
        class CrioPullImageAsyncOperation;
        class CrioCreateContainerAsyncOperation;
        class CrioStartContainerAsyncOperation;
        class CrioStopContainerAsyncOperation;
        class CrioRemoveContainerAsyncOperation;
        class CrioGetContainerLogAsyncOperation;
        class CrioGetContainerLogFromFilePathAsyncOperation;
        class InitializeAsyncOperation;

        static runtime::PodSandboxConfig* genPodSandboxConfig(std::string const & name,
                                                              std::string const & cgparent,
                                                              std::string const & hostname,
                                                              std::string const & logdirectory,
                                                              ServiceModel::NetworkType::Enum networkType,
                                                              bool isdnsserviceenabled,
                                                              std::string const & dnssearchoptions,
                                                              vector<string> & dnsServers,
                                                              std::vector<std::tuple<std::string,int,int,int>> const &portmappings);
        static runtime::ContainerConfig* genContainerConfig(std::string const &name, std::string const &imgname,
                                                            vector<string> const & cmds, vector<string> const & args,
                                                            vector<pair<string, string>> const & envs,
                                                            vector<tuple<string, string, bool>> const & mounts,
                                                            vector<pair<string, string>> const & labels,
                                                            string const & workdir, string const & logpath, bool interactive);

        static void printPodSandboxConfig(runtime::PodSandboxConfig* podSandboxConfig);

    private:
        std::shared_ptr<grpc::Channel> rpcChannel_;
        std::unique_ptr<runtime::RuntimeService::Stub> rtStub_;
        std::unique_ptr<runtime::ImageService::Stub> imgStub_;
        grpc::CompletionQueue compQueue_;
    };
};
