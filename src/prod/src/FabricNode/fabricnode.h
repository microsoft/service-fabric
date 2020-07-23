// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Federation/Federation.h"
#include "Hosting2/Hosting.h"
#include "Reliability/Reliability.h"
#include "Naming/naming.h"
#include "Communication/Communication.h"
#include "Management/BackupRestore/BA/BA.h"
#include "Management/ManagementSubsystem/ManagementSubsystem.h"

#include "ktllogger/ktlloggerX.h"

namespace Fabric
{
    class FabricNode;
    typedef std::shared_ptr<FabricNode> FabricNodeSPtr;
    typedef std::weak_ptr<FabricNode> FabricNodeWPtr;

    class FabricNode
        : public Common::ComponentRoot
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::FabricNode>
    {
    public:
        static Common::ErrorCode Create(
            Common::FabricNodeConfigSPtr const & config,
            FabricNodeSPtr & fabricNodeSPtr);

        // Function for creating fabric node from test code
        static Common::ErrorCode Test_Create(
            Common::FabricNodeConfigSPtr const & config,
            Hosting2::HostingSubsystemFactoryFunctionPtr hostingFactory,
            Common::ProcessTerminationServiceFactory processTerminationServiceFactory,
            FabricNodeSPtr & fabricNodeSPtr);

        void CreateDataDirectoriesIfNeeded();
        Common::ErrorCode RemoveDataDirectories();

        // Create and open a FabricNode, return the opened FabricNode. If there is recoverable failure during the process, destroy the
        // current FabricNode instance, create a new one and retry open until success or time runs out. There is delay between retries.
        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            Common::AsyncCallback const & callback,
            Common::EventHandler const & nodeFailEventHandler,
            Common::EventHandler const & fmReadyEventHandler);

        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            Common::FabricNodeConfigSPtr const & config,
            Common::AsyncCallback const & callback,
            Common::EventHandler const & nodeFailEventHandler,
            Common::EventHandler const & fmReadyEventHandler);

        static Common::AsyncOperationSPtr BeginCreateAndOpen(
            Common::FabricNodeConfigSPtr const & config,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::EventHandler const & nodeFailEventHandler,
            Common::EventHandler const & fmReadyEventHandler);

        static Common::ErrorCode EndCreateAndOpen(Common::AsyncOperationSPtr const & operation, __out FabricNodeSPtr & fabricNode);

        Common::AsyncOperationSPtr BeginClose(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & operation);

        virtual ~FabricNode();

        __declspec (property(get=get_NodeInstance)) Federation::NodeInstance const & Instance;
        Federation::NodeInstance const & get_NodeInstance() const {return federation_->Instance;}

        __declspec (property(get=getNodeId)) Federation::NodeId Id;
        Federation::NodeId getNodeId() const {return federation_->Id;}

        __declspec (property(get=Test_get_Hosting)) Hosting2::IHostingSubsystem & Test_Hosting;
        Hosting2::IHostingSubsystem & Test_get_Hosting() const { return *hosting_; }

        __declspec (property(get=Test_get_RuntimeListenAddress)) std::wstring const& Test_RuntimeListenAddress;
        std::wstring const& Test_get_RuntimeListenAddress() const { return ipcServer_->TransportListenAddress; }

        __declspec (property(get=Test_get_Reliability)) Reliability::IReliabilitySubsystem & Test_Reliability;
        Reliability::IReliabilitySubsystem & Test_get_Reliability() const { return *reliability_; }

        __declspec (property(get=Test_get_Communication)) Communication::CommunicationSubsystem & Test_Communication;
        Communication::CommunicationSubsystem & Test_get_Communication() const { return *communication_; }

        __declspec (property(get=Test_get_Management)) Management::ManagementSubsystem & Test_Management;
        Management::ManagementSubsystem & Test_get_Management() const { return *management_; }

        __declspec (property(get = Test_get_RuntimeSharingHelper)) Hosting2::IRuntimeSharingHelper & Test_RuntimeSharingHelper;
        Hosting2::IRuntimeSharingHelper & Test_get_RuntimeSharingHelper() const { return *runtimeSharingHelper_; }

        // This is only valid after FabricNode's constructor finishes
        __declspec (property(get=get_IsInZombieMode)) bool IsInZombieMode;
        bool get_IsInZombieMode() const { return zombieModeEntreeService_ != nullptr; }

        Common::FabricNodeConfigSPtr const & Config() const { return config_; }

        Common::HHandler RegisterFailoverManagerReadyEvent(Common::EventHandler handler);
        bool UnRegisterFailoverManagerReadyEvent(Common::HHandler hHandler);

        Common::HHandler RegisterNodeFailedEvent(Common::EventHandler handler);
        bool UnRegisterNodeFailedEvent(Common::HHandler hHandler);

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:
        FabricNode(
            Common::FabricNodeConfigSPtr const & config, 
            Hosting2::HostingSubsystemFactoryFunctionPtr hostingSubsystemFactory,
            Common::ProcessTerminationServiceFactory processTerminationFactory);

        Common::ErrorCode CreateClusterSecuritySettings(Transport::SecuritySettings & clusterSecuritySettings);
        Common::ErrorCode CreateIpcServerTlsSecuritySettings(Transport::SecuritySettings & ipcServerTlsSecuritySettings);

        void RegisterEvents();
        void UnRegisterEvents();
        void OnNodeFailed();
        void OnFailoverManagerReady();

        bool TryStartSystemServiceOperation(Common::AsyncOperationSPtr const & operation);
        void TryStartInitializeSystemService(Common::AsyncOperationSPtr const & operation);
        void StartInitializeNamingService();
        void StartInitializeClusterManagerService();
        void StartInitializeRepairManagerService();
        void StartInitializeSystemServices();
        void StartInitializeImageStoreService();
        void StartInitializeFaultAnalysisService();
        void StartInitializeUpgradeOrchestrationService();
        void StartInitializeCentralSecretService();
        void StartInitializeBackupRestoreService();
        void StartInitializeEventStoreService();

        void TraceState();
        Common::DateTime DetermineStoppedNodeDuration();
        void ExitZombieMode();
        void CacheStaticTraceStates();
        void StartTracingTimer();

        void InitializeStopExpirationTimer();

        void CancelTimers();

        void RegisterForConfigUpdate();
        static void ClusterSecuritySettingUpdateHandler(std::weak_ptr<Common::ComponentRoot> const & rootWPtr);
        Common::ErrorCode OnClusterSecuritySettingUpdated();
        Common::ErrorCode SetClusterSecurity(Transport::SecuritySettings const & securitySettings);

        Common::ErrorCode SetClusterSpnIfNeeded();

        void EnableTransportHealthReporting();
        void ReportTransportHealth(
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & dynamicProperty,
            std::wstring const & property,
            Common::TimeSpan ttl);

        void ReportLeaseHealth(int reportCode, wstring const & dynamicProperty, wstring const & extraDescription);

        void EnableCrlHealthReportIfNeeded();
        void ReportCrlHealth(size_t cacheSize, std::wstring const & description); 

        static void CreateDataDirectoryIfNeeded(std::wstring const & directory, std::wstring const & description);
        static Common::ErrorCode RemoveDataDirectory(std::wstring const & directory, std::wstring const & description);

        void InitializeZombieMode(Federation::FederationSubsystem &, Transport::SecuritySettings const &);

        bool IsZombieModeFilePresent();

    private:
        Common::FabricNodeConfigSPtr config_;

        Federation::FederationSubsystemSPtr federation_;
	    KtlLogger::KtlLoggerNodeSPtr ktlLogger_;
        Transport::IpcServerUPtr ipcServer_;
        Hosting2::IHostingSubsystemSPtr hosting_;
        Hosting2::IRuntimeSharingHelperUPtr runtimeSharingHelper_;
        Reliability::IReliabilitySubsystemUPtr reliability_;
        Hosting2::NetworkInventoryAgentSPtr networkInventoryAgent_;
#if !defined(PLATFORM_UNIX)
        Management::BackupRestoreAgentComponent::BackupRestoreAgentUPtr backupRestoreAgent_;
#endif
        Communication::CommunicationSubsystemUPtr communication_;
        std::unique_ptr<Management::ManagementSubsystem> management_;
        Client::HealthReportingComponentSPtr healthClient_;
        Testability::ITestabilitySubsystemSPtr testability_;

        Common::TimerSPtr tracingTimer_;

        Common::TimeSpan stateTraceInterval_;
        std::wstring faultDomainString_;
        std::wstring addressString_;
        std::wstring hostNameString_;
        bool isSeedNode_;
        bool shouldReportCrlHealth_ = false;

        Common::HHandler nodeFailedHHandler_;
        Common::HHandler fmReadyHHandler_;
        Common::HHandler fmFailedHHandler_;

        Common::Event fmReadyEvent_;
        Common::Event nodeFailedEvent_;

        std::vector<Common::AsyncOperationSPtr> activeSystemServiceOperations_;
        Common::RwLock timerLock_;

        std::wstring workingDir_;

        Common::ErrorCode constructError_;

        Common::DateTime clusterCertExpiration_;

        std::unique_ptr<Naming::EntreeService> zombieModeEntreeService_;
        Common::TimerSPtr stopNodeExpiredTimer_;

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class CreateAndOpenAsyncOperation;
        class InitializeSystemServiceAsyncOperationBase;
        class InitializeNamingServiceAsyncOperation;
        class InitializeClusterManagerServiceAsyncOperation;
        class InitializeRepairManagerServiceAsyncOperation;
        class InitializeImageStoreServiceAsyncOperation;        
        class InitializeFaultAnalysisServiceAsyncOperation;
        class DummyIRouter;
        class InitializeUpgradeOrchestrationServiceAsyncOperation;
        class InitializeCentralSecretServiceAsyncOperation;
        class InitializeBackupRestoreServiceAsyncOperation;
        class InitializeEventStoreServiceAsyncOperation;

        Common::RwLock clusterSecuritySettingsUpdateLock_;
    };
}
