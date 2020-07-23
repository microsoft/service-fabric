// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetContainerInfoRequest;
    class ProcessActivationManager
        : public Common::RootedObject
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ProcessActivationManager)

    public:
        ProcessActivationManager(Common::ComponentRoot const & root,
            FabricActivationManager const & activationManager);

        virtual ~ProcessActivationManager();

        _declspec(property(get=get_ProcessActivator)) ProcessActivatorUPtr const & ProcessActivatorObj;
        inline ProcessActivatorUPtr const & get_ProcessActivator() const { return activationManager_.ProcessActivatorObj; }

        _declspec(property(get = get_ContainerActivatorProxy)) IContainerActivatorUPtr const & ContainerActivatorObj;
        inline IContainerActivatorUPtr const & get_ContainerActivatorProxy() const { return containerActivator_; }

        _declspec(property(get = get_ActivationManager)) FabricActivationManager const & ActivationManager;
        inline FabricActivationManager const & get_ActivationManager() const { return activationManager_; }

        _declspec(property(get = get_FirewallSecurityProvider)) FirewallSecurityProviderUPtr const & FirewallSecurityProviderObj;
        inline FirewallSecurityProviderUPtr const & get_FirewallSecurityProvider() const { return firewallProvider_; }

        _declspec(property(get = get_IsSystem)) bool IsSystem;
        inline bool get_IsSystem() const { return isSystem_; }

        void OnContainerTerminated(
            std::wstring const & appServiceId,
            std::wstring const & nodeId,
            DWORD exitCode);

        void OnContainerHealthCheckStatusEvent(wstring const & nodeId, vector<ContainerHealthStatusInfo> const & healthStatusInfoList);

        void OnContainerActivatorServiceTerminated();

        //this method compares process description object content with the actual limits on cgroup/job object level
        //and returns back all the cpu/memory limits found
        //first vector contains all of the cpu limits on CP level for all CPs
        //second vector containes all of the memory limits
        bool Test_VerifyLimitsEnforced(std::vector<double> & cpuLimitCP, std::vector<uint64> & memoryLimitCP) const;

        //for each pod, for each container do a docker inspect and verify namespaces, cgroup parent etc
        //vector contains the sizes of container groups
        bool Test_VerifyPods(std::vector<int> & containerGroups) const;

        Common::AsyncOperationSPtr BeginGetOverlayNetworkDefinition(
            std::wstring const & networkName,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndGetOverlayNetworkDefinition(
            Common::AsyncOperationSPtr const & operation,
            OverlayNetworkDefinitionSPtr & networkDefinition);

        Common::AsyncOperationSPtr BeginDeleteOverlayNetworkDefinition(
            std::wstring const & networkName,
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDeleteOverlayNetworkDefinition(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginPublishNetworkTablesRequest(
            std::wstring const & networkName,
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndPublishNetworkTablesRequest(
            Common::AsyncOperationSPtr const & operation);

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);


        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & , 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:  
        Common::ErrorCode CheckAddParentProcessMonitoring(
            DWORD parentId,
            std::wstring const & nodeId,
            std::wstring const & callbackAddress,
            uint64 nodeInstanceId);

        void OnParentProcessTerminated(
            DWORD parentId,
            std::wstring const & nodeId,
            Common::ErrorCode const & waitResult,
            uint64 nodeInstanceId);

        void OnApplicationServiceTerminated(
            DWORD processId,
            Common::DateTime const & startTime,
            Common::ErrorCode const & waitResult,
            DWORD exitCode,
            std::wstring const & parentId,
            std::wstring const & appServiceId,
            std::wstring const & exePath);

        void RegisterIpcRequestHandler();
        void UnregisterIpcRequestHandler();

        Common::ErrorCode TerminateAndCleanup(std::wstring const & nodeId, DWORD processId, uint64 nodeInstanceId);

        void CleanupPortAclsForNode(std::wstring const & nodeId);

        void ProcessIpcMessage(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessActivateProcessRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDeactivateProcessRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void SendDeactivateProcessReply(
            std::wstring const & appServiceId,
            Common::ErrorCode const & error,
            __in Transport::IpcReceiverContextUPtr & context);

         void ProcessTerminateProcessRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

       void ProcessRegisterClientRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

       void SendRegisterClientReply(
           Common::ErrorCode const & error,
           __in Transport::IpcReceiverContextUPtr & context);

        void ProcessConfigureSecurityPrincipalRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void SendConfigureSecurityPrincipalReply(
            std::vector<SecurityPrincipalInformationSPtr> const &,
            Common::ErrorCode const &,
            __in Transport::IpcReceiverContextUPtr & context);

         void ProcessConfigureResourceACLsRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

          void ProcessConfigureEndpointSecurityRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessUnregisterActivatorClientRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessFabricUpgradeRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessCreateSymbolicLinkRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessConfigureSharedFolderRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessConfigureSMBShareRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessConfigureContainerCertificateExportRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessCleanupContainerCertificateExportRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDownloadContainerImagesMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDeleteContainerImagesMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessGetContainerInfoMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessGetResourceUsage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessGetImagesMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessConfigureNodeForDnsServiceMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessAssignIPAddressesRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessManageOverlayNetworkResourcesRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessUpdateOverlayNetworkRoutesRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessGetDeployedNetworkCodePackagesRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessGetDeployedNetworksRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessSetupContainerGroupRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessConfigureEndpointCertificatesAndFirewallPolicyRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendFabricActivatorOperationReply(
            Common::ErrorCode const & error,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendCleanupApplicationPrincipalsReply(
            Common::ErrorCode const & error,
            std::vector<std::wstring> const & deletedAppIds, 
            __in Transport::IpcReceiverContextUPtr & context);

        void SendConfigureContainerCertificateExportReply(
            Common::ErrorCode const & error,
            std::map<std::wstring, std::wstring> certificatePaths,
            std::map<std::wstring, std::wstring> certificatePasswordPaths,
            __in Transport::IpcReceiverContextUPtr & context);

#if defined(PLATFORM_UNIX)
        void SendDeleteApplicationFoldersReply(
            Common::ErrorCode const & error,
            std::vector<std::wstring> const & deletedAppFolders,
            __in Transport::IpcReceiverContextUPtr & context);

        Common::ErrorCode CopyAndACLPEMFromStore(
             wstring const & x509FindValue,
             wstring const & x509StoreName,
             wstring const & destinationPath,
             vector<wstring> const & machineAccountNameForACL,
             __out wstring & privateKeyPath);
#endif
       void FinishFabricUpgradeRequest(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

       void FinishSendServiceTerminationMessage(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
       void OnActivateServiceCompleted(Common::AsyncOperationSPtr const & operation);
       void OnDeactivateServiceCompleted(Common::AsyncOperationSPtr const & operation);
       void OnContainerImagesOperationCompleted(Common::AsyncOperationSPtr const & operation, bool isDeleteOperation);
       void OnContainerGroupSetupCompleted(Common::AsyncOperationSPtr const & operation);
       void OnGetContainerInfoOperationCompleted(Common::AsyncOperationSPtr const & operation);
       void OnGetResourceUsageComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously);
       void OnGetImagesCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously);
       void OnContainerEngineTerminated(DWORD exitCode, ULONG continuousFailureCount, Common::TimeSpan const& nextStartTime);
       Common::ErrorCode CopyAndACLCertificateFromDataPackage(
           wstring const & dataPackageName,
           wstring const & dataPackageVersion,
           wstring const & relativePath,
           wstring const & destinationPath,
           std::vector<wstring> const & machineAccountNamesForACL);

       Common::ErrorCode ConfigureFolderACLs(
           std::vector<std::wstring> const & applicationFolders,
           DWORD accessMask,
           std::vector<std::wstring> const & sids,
           Common::TimeoutHelper const & timeoutHelper);

       void ProcessRemoveApplicationGroupRequest(
           __in Transport::Message & message,
           __in Transport::IpcReceiverContextUPtr & context);

#if defined(PLATFORM_UNIX)
       void ProcessDeleteApplicationFolderRequest(
           __in Transport::Message & message,
           __in Transport::IpcReceiverContextUPtr & context);
#endif

       bool IsRequestorRegistered(std::wstring const & requestorId);

#if defined(PLATFORM_UNIX)
       void UnmapClearContainerVolumes(ApplicationServiceSPtr const & appService);
       void OnUnmapAllVolumesCompleted(
           Common::AsyncOperationSPtr const & operation,
           bool expectedCompletedSynchronously);
#endif

       void SendApplicationServiceTerminationNotification(
           std::wstring const & parentId,
           std::wstring const & appServiceId,
           DWORD exitCode,
           Common::ActivityDescription const & activityDescription,
           bool isRootContainer);
       void SendApplicationServiceTerminationNotification(
           Common::ActivityDescription const & activityDescription,
           std::wstring const & parentId,
           std::vector<std::wstring> const & appServiceIds,
           DWORD exitCode,
           bool isRootContainer);

       void SendContainerHealthCheckStatusNotification(
           std::wstring const & nodeId,
           std::vector<ContainerHealthStatusInfo> const & healthStatusInfoList);
       void FinishSendContainerHealthCheckStatusMessage(
           Common::AsyncOperationSPtr const & operation, 
           bool expectedCompletedSynchronously);

       Common::ErrorCode ConfigureCertificateACLs(
           std::vector<CertificateAccessDescription> const &,
           Common::TimeoutHelper const & );    

       void OnTerminateServicesCompleted(Common::AsyncOperationSPtr const&);

       Common::ErrorCode ConfigureEndpointCertificates(
           std::vector<EndpointCertificateBinding> const & endpointCertificateBindings,
           std::wstring const & servicePackageId,
           bool cleanUp,
           std::wstring const & nodeId);

       Common::ErrorCode ProcessActivationManager::ConfigureFirewallPolicy(
           bool removeRule,
           std::wstring const & servicePackageId,
           std::vector<LONG> const & ports,
           uint64 nodeInstanceId);

       void ScheduleTimerForStats(Common::TimeSpan const& timeSpan);
       void PostStatistics(Common::TimerSPtr const & timer);

       void SendDockerProcessTerminationNotification(DWORD exitCode, ULONG continuousFailureCount, Common::TimeSpan const& nextStartTime);
       void FinishSendDockerProcessTerminationNotification(
           Common::AsyncOperationSPtr const & operation,
           bool expectedCompletedSynchronously);

    private:

        FabricActivationManager const & activationManager_;
  
        BOOL isSystem_;

        ApplicationServiceMapSPtr applicationServiceMap_;
        SecurityIdentityMap<ActivatorRequestorSPtr> requestorMap_;

        SecurityPrincipalsProviderUPtr principalsProvider_;
        HttpEndpointSecurityProviderUPtr endpointProvider_;
        CrashDumpConfigurationManagerUPtr crashDumpConfigurationManager_;
        FirewallSecurityProviderUPtr firewallProvider_;
        IContainerActivatorUPtr containerActivator_;

        Common::RwLock lock_;
        Common::RwLock registryAccessLock_;

        Common::TimerSPtr statisticsIntervalTimerSPtr_;

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class DeactivateServiceAsyncOperation;
        class ActivateServiceAsyncOperation;
        class NotifyServiceTerminationAsyncOperation;
        class FabricUpgradeAsyncOperation;
        class DownloadContainerImagesAsyncOperation;
        class DeleteContainerImagesAsyncOperation;
        class ActivateContainerGroupRootAsyncOperation;
        class TerminateServicesAsyncOperation;
        class GetContainerInfoAsyncOperation;
        class NotifyContainerHealthCheckStatusAsyncOperation;
        class GetResourceUsageAsyncOperation;
        class GetImagesAsyncOperation;
        class NotifyDockerProcessTerminationAsyncOperation;
        class GetOverlayNetworkDefinitionAsyncOperation;
        class DeleteOverlayNetworkDefinitionAsyncOperation;
        class PublishNetworkTablesAsyncOperation;

        friend class ApplicationService;
        friend class ActivatorRequestor;
    };
}