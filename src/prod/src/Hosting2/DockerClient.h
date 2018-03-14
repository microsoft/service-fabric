// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace web
{
    namespace http
    {
        namespace client
        {
             class http_client;
        }
    }
}

namespace Hosting2 
{
    class DockerClient : protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:

        DockerClient(
            std::string const & serverAddress,
            ProcessActivationManager & processActivationManager); 

        ~DockerClient();

        bool Initialize();
        
        void AddContainerToMap(
            std::wstring const & containerName,
            std::wstring const & appHostId,
            std::wstring const & nodeId,
            bool reportDockerHealthStatus);
        
        void ClearContainerMap();

    private:
        struct ContainerInfo;
        class ContainerEventManager;

        typedef std::unique_ptr<ContainerEventManager> ContainerEventManagerUPtr;
        
        bool CheckInitializeDocker();
        void NotifyOnContainerTermination(std::wstring const & containerName, DWORD exitCode);

        void ProcessContainerEvents(std::vector<ContainerEventConfig> && events);
        
        bool TryGetNodeIdAndAppHostId(
            __in std::wstring const & containerName,
            __out std::wstring & nodeId,
            __out std::wstring & appHostId);
        
        bool ShouldReportDockerHealthStatus(wstring const & containerName);
        
        void NotifyOnContainerHealthStatus(
            std::wstring const & nodeId, 
            std::vector<ContainerHealthStatusInfo> const & healthStatusInfo);

    private:
        std::unique_ptr<web::http::client::http_client> httpClient_;
        ContainerEventManagerUPtr eventManager_;
        volatile bool isInitialized_;
        volatile bool isEventManagerRegistered_;
        std::map<std::wstring, ContainerInfo> containerIdMap_;
        Common::ExclusiveLock mapLock_;
        Common::ExclusiveLock initializerLock_;
        ProcessActivationManager & processActivationManager_;
        std::string serverAddress_;
    };
}
