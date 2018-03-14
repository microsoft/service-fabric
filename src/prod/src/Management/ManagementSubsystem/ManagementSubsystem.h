// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Management/ClusterManager/ClusterManager.h"
#include "Management/ImageStore/ImageStoreServiceConfig.h"
#include "Management/FileStoreService/Constants.h"
#include "Management/RepairManager/RepairManagerConfig.h"
#include "Management/RepairManager/Constants.h"
#include "Management/UpgradeService/UpgradeServiceConfig.h"
#include "Management/BackupRestoreService/BackupRestoreServiceConfig.h"
#include "Management/FaultAnalysisService/FaultAnalysisServiceConfig.h"
#include "Management/FaultAnalysisService/Constants.h"
#include "Management/UpgradeOrchestrationService/UpgradeOrchestrationServiceConfig.h"
#include "Management/UpgradeOrchestrationService/Constants.h"
#include "Management/DnsService/include/DnsEventSource.h"
#include "Management/DnsService/config/DnsServiceConfig.h"
#include "Management/ResourceMonitor/config/ResourceMonitorServiceConfig.h"

namespace Management
{
    class ManagementSubsystem : 
        public Common::FabricComponent,
        private Common::RootedObject,
        public Common::TextTraceComponent<Common::TraceTaskCodes::FabricNode>
    {
    public:
        ManagementSubsystem(
            Reliability::FederationWrapper &,
            Hosting2::IRuntimeSharingHelper &,
            Hosting2::IFabricActivatorClientSPtr const &,
            Reliability::ServiceAdminClient &,
            Reliability::ServiceResolver &,
            std::wstring const & clientConnectionAddress,
            Api::IClientFactoryPtr &&,
            std::wstring const & clusterManagerReplicatorAddress,
            std::wstring const & workingDir,
            std::wstring const & nodeName,
            Transport::SecuritySettings const& clusterSecuritySettings,
            Common::ComponentRoot const &);

        ~ManagementSubsystem();

        __declspec(property(get=get_IsEnabled)) bool IsEnabled;
        bool get_IsEnabled() const;

        __declspec(property(get=get_IsCMEnabled)) bool IsCMEnabled;
        bool get_IsCMEnabled() const;

        __declspec(property(get=get_IsRMEnabled)) bool IsRMEnabled;
        bool get_IsRMEnabled() const;

        __declspec(property(get=get_IsISEnabled)) bool IsISEnabled;
        bool get_IsISEnabled() const;

        __declspec(property(get=get_IsTVSEnabled)) bool IsTVSEnabled;
        bool get_IsTVSEnabled() const;

        __declspec(property(get=get_IsImageStoreServiceEnabled)) bool IsImageStoreServiceEnabled;
        bool get_IsImageStoreServiceEnabled() const;

        __declspec(property(get = get_IsUpgradeServiceEnabled)) bool IsUpgradeServiceEnabled;
        bool get_IsUpgradeServiceEnabled() const;
        
        __declspec(property(get=get_IsFaultAnalysisServiceEnabled)) bool IsFaultAnalysisServiceEnabled;
        bool get_IsFaultAnalysisServiceEnabled() const;

        __declspec(property(get = get_IsBackupRestoreServiceEnabled)) bool IsBackupRestoreServiceEnabled;
        bool get_IsBackupRestoreServiceEnabled() const;

        __declspec(property(get=Test_get_ClusterManagerFactory)) ClusterManager::ClusterManagerFactory & Test_ClusterManagerFactory;
        ClusterManager::ClusterManagerFactory & Test_get_ClusterManagerFactory() const;

        __declspec(property(get=get_RuntimeSharingHelper)) Hosting2::IRuntimeSharingHelper & Test_RuntimeSharingHelper;
        Hosting2::IRuntimeSharingHelper & get_RuntimeSharingHelper() const;

        __declspec(property(get=Test_get_ActiveServices)) Management::ClusterManager::Test_ReplicaMap const & Test_ActiveServices;
        Management::ClusterManager::Test_ReplicaMap const & Test_get_ActiveServices() const;

        __declspec(property(get = get_IsUpgradeOrchestrationServiceEnabled)) bool IsUpgradeOrchestrationServiceEnabled;
        bool get_IsUpgradeOrchestrationServiceEnabled() const;

        __declspec(property(get = get_IsDnsServiceEnabled)) bool IsDnsServiceEnabled;
        bool get_IsDnsServiceEnabled() const;

        //
        // Cluster Manager Service
        //

        Common::AsyncOperationSPtr BeginCreateClusterManagerService(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateClusterManagerService(Common::AsyncOperationSPtr const &);

        static Reliability::ServiceDescription CreateClusterManagerServiceDescription();

        static Reliability::ServiceDescription CreateClusterManagerServiceDescription(
            int targetReplicaSetSize, 
            int minReplicaSetSize, 
            Common::TimeSpan replicaRestartWaitDuration,
            Common::TimeSpan standByReplicaKeepDuration,
            Common::TimeSpan quorumLossWaitDuration,
            std::wstring const& placementConstraints);

        //
        // Repair Manager Service
        //

        Common::AsyncOperationSPtr BeginCreateRepairManagerService(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateRepairManagerService(Common::AsyncOperationSPtr const &);

        static Reliability::ServiceDescription CreateRepairManagerServiceDescription();

        //
        // Image Store Service
        //

        Common::AsyncOperationSPtr BeginCreateImageStoreService(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateImageStoreService(Common::AsyncOperationSPtr const &);

        static Reliability::ServiceDescription CreateImageStoreServiceDescription();

        //
        // Infrastructure Service, Upgrade Service, Token Validation Service
        //

        Common::AsyncOperationSPtr BeginInitializeSystemServices(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndInitializeSystemServices(Common::AsyncOperationSPtr const &);

        static void LoadAllSystemServiceDescriptions(std::vector<Reliability::ServiceDescription> &);
        static void LoadSystemServiceDescriptions(std::vector<Reliability::ServiceDescription> &, std::wstring const & nameFilter);

        static Reliability::ServiceDescription CreateUpgradeOrchestrationServiceDescription();
        static Reliability::ServiceDescription CreateUpgradeServiceDescription(std::wstring const & serviceName);
        static Reliability::ServiceDescription CreateBackupRestoreServiceDescription();
        static Reliability::ServiceDescription CreateFaultAnalysisServiceDescription();

        Common::AsyncOperationSPtr BeginCreateUpgradeOrchestrationService(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateUpgradeOrchestrationService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateFaultAnalysisService(
               Common::TimeSpan const,
               Common::AsyncCallback const &,
               Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateFaultAnalysisService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateBackupRestoreService(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateBackupRestoreService(Common::AsyncOperationSPtr const &);

        Common::ErrorCode SetClusterSecurity(Transport::SecuritySettings const & value);
        Common::ErrorCode SetKeepAlive(ULONG keepAliveIntervalInSeconds);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        class Impl;
        class ServiceConfigBase;
        class InfrastructureServiceConfig;
        class TokenValidationServiceConfig;
        class CreateSystemServiceAsyncOperation;
        class DeleteSystemServiceAsyncOperation;
        class InitializeSystemServiceAsyncOperation;

        static void AddInfrastructureServiceDescription(std::wstring const & serviceName, __in std::vector<Reliability::ServiceDescription> &);
        static void AddTokenValidationServiceDescription(std::wstring const & serviceName, __in std::vector<Reliability::ServiceDescription> &);
        static void AddDnsServiceDescription(std::wstring const & serviceName, __in std::vector<Reliability::ServiceDescription> &);
        static void AddResourceMonitorServiceDescription(std::wstring const & serviceName, __in std::vector<Reliability::ServiceDescription> &);

        std::unique_ptr<Impl> impl_;
    };
}
