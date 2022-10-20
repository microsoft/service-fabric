// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "client/ClientServerTransport/CreateComposeDeploymentRequestHeader.h" //move this to stdafx.h when also needed by other cpps in the component

using namespace Common;
using namespace Hosting2;
using namespace Reliability;
using namespace ServiceModel;
using namespace Store;
using namespace Transport;
using namespace Naming;
using namespace Management::ClusterManager;
using namespace Management::InfrastructureService;
using namespace Management::FileStoreService;
using namespace Management::ImageStore;
using namespace Management::RepairManager;
using namespace Management::UpgradeService;
using namespace Management::BackupRestoreService;
using namespace Management::FaultAnalysisService;
using namespace Management::UpgradeOrchestrationService;
using namespace Management::CentralSecretService;
using namespace Management::LocalSecretService;
using namespace Management::GatewayResourceManager;
using namespace std;
using namespace Api;
using namespace Client;
using namespace DNS;
using namespace ClientServerTransport;
using namespace ResourceMonitor;

namespace Management
{
    static StringLiteral const TraceManagementSubsystem("ManagementSubsystem");

    class ManagementSubsystem::ServiceConfigBase
    {
    protected:

        ServiceConfigBase()
            : sectionName_()
            , placementConstraints_()
        {
        }

    public:

        __declspec(property(get = get_SectionName)) wstring const& SectionName;
        inline wstring const& get_SectionName() const { return sectionName_; }

        __declspec(property(get = get_PlacementConstraints)) wstring const& PlacementConstraints;
        inline wstring const& get_PlacementConstraints() const { return placementConstraints_; }

        virtual bool TryReadFromConfig(wstring const& sectionName, StringMap const& configSection)
        {
            sectionName_ = sectionName;

            auto it = configSection.find(L"PlacementConstraints");
            if (it != configSection.end())
            {
                placementConstraints_ = it->second;
            }

            return true;
        }

    private:
        wstring sectionName_;
        wstring placementConstraints_;
    };

    class ManagementSubsystem::InfrastructureServiceConfig : public ServiceConfigBase
    {
    public:
        InfrastructureServiceConfig()
            : serviceName_()
            , targetReplicaSetSize_(0)
            , minReplicaSetSize_(0)
        {
        }

        __declspec(property(get = get_ServiceName)) wstring const& ServiceName;
        inline wstring const& get_ServiceName() const { return serviceName_; }

        __declspec(property(get = get_TargetReplicaSetSize)) int TargetReplicaSetSize;
        inline int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

        __declspec(property(get = get_MinReplicaSetSize)) int MinReplicaSetSize;
        inline int get_MinReplicaSetSize() const { return minReplicaSetSize_; }

        bool TryReadFromConfig(wstring const& sectionName, StringMap const& configSection) override
        {
            if (!ServiceConfigBase::TryReadFromConfig(sectionName, configSection))
            {
                return false;
            }

            // Canonicalize casing of service name (given by config section name)
            auto separatorPos = sectionName.find(L'/');
            if (separatorPos == wstring::npos)
            {
                // Default instance
                serviceName_ = *SystemServiceApplicationNameHelper::InternalInfrastructureServiceName;
            }
            else if (separatorPos < sectionName.size() - 1)
            {
                // Named instance
                wstring instanceName = sectionName.substr(separatorPos + 1);
                serviceName_ = *SystemServiceApplicationNameHelper::InternalInfrastructureServiceName + L"/" + instanceName;
            }
            else
            {
                // Instance name is empty string, which is not allowed
                return false;
            }

            auto it = configSection.find(L"TargetReplicaSetSize");
            if (it != configSection.end() && !Config::TryParse<int>(targetReplicaSetSize_, it->second))
            {
                return false;
            }

            it = configSection.find(L"MinReplicaSetSize");
            if (it != configSection.end() && !Config::TryParse<int>(minReplicaSetSize_, it->second))
            {
                return false;
            }

            return true;
        }

    private:
        wstring serviceName_;
        int minReplicaSetSize_;
        int targetReplicaSetSize_;
    };

    class ManagementSubsystem::TokenValidationServiceConfig : public ServiceConfigBase
    {
    };

    class ManagementSubsystem::EventStoreServiceConfig : public ServiceConfigBase
    {
    public:
        EventStoreServiceConfig()
            : targetReplicaSetSize_(0)
            , minReplicaSetSize_(0)
        {
        }

        __declspec(property(get = get_TargetReplicaSetSize)) int TargetReplicaSetSize;
        inline int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

        __declspec(property(get = get_MinReplicaSetSize)) int MinReplicaSetSize;
        inline int get_MinReplicaSetSize() const { return minReplicaSetSize_; }

        bool TryReadFromConfig(wstring const& sectionName, StringMap const& configSection) override
        {
            if (!ServiceConfigBase::TryReadFromConfig(sectionName, configSection))
            {
                return false;
            }

            int configTargetReplicaSetSize = 0;
            int configMinReplicaSetSize = 0;

            auto it = configSection.find(L"TargetReplicaSetSize");
            if (it == configSection.end() || !Config::TryParse<int>(configTargetReplicaSetSize, it->second))
            {
                return false;
            }

            it = configSection.find(L"MinReplicaSetSize");
            if (it == configSection.end() || !Config::TryParse<int>(configMinReplicaSetSize, it->second))
            {
                return false;
            }

            targetReplicaSetSize_ = configTargetReplicaSetSize;
            minReplicaSetSize_ = configMinReplicaSetSize;

            return true;
        }

    private:
        int minReplicaSetSize_;
        int targetReplicaSetSize_;
    };

    class ManagementSubsystem::Impl
        : public RootedObject
    {
        DENY_COPY(Impl);

    public:
        Impl(
            FederationWrapper &,
            Hosting2::IRuntimeSharingHelper &,
            Hosting2::IFabricActivatorClientSPtr const &,
            ServiceAdminClient &,
            ServiceResolver &,
            wstring const & clientConnectionAddress,
            IClientFactoryPtr &&,
            wstring const & clusterManagerReplicatorAddress,
            wstring const & workingDir,
            wstring const & nodeName,
            Transport::SecuritySettings const& clusterSecuritySettings,
            ComponentRoot const & root);

        __declspec(property(get = get_IsEnabled)) bool IsEnabled;
        bool get_IsEnabled() const { return this->IsCMEnabled; }

        __declspec(property(get = get_IsCMEnabled)) bool IsCMEnabled;
        bool get_IsCMEnabled() const { return (ManagementConfig::GetConfig().TargetReplicaSetSize > 0); }

        __declspec(property(get = get_IsRMEnabled)) bool IsRMEnabled;
        bool get_IsRMEnabled() const { return (RepairManagerConfig::GetConfig().TargetReplicaSetSize > 0); }

        __declspec(property(get = get_IsISEnabled)) bool IsISEnabled;
        bool get_IsISEnabled() const;

        __declspec(property(get = get_IsTVSEnabled)) bool IsTVSEnabled;
        bool get_IsTVSEnabled() const;

        __declspec(property(get = get_IsImageStoreServiceEnabled)) bool IsImageStoreServiceEnabled;
        bool get_IsImageStoreServiceEnabled() const;

        __declspec(property(get = get_IsUpgradeServiceEnabled)) bool IsUpgradeServiceEnabled;
        bool get_IsUpgradeServiceEnabled() const;

        __declspec(property(get = get_IsFaultAnalysisServiceEnabled)) bool IsFaultAnalysisServiceEnabled;
        bool get_IsFaultAnalysisServiceEnabled() const { return (FaultAnalysisServiceConfig::GetConfig().TargetReplicaSetSize > 0); }

        __declspec(property(get = get_IsBackupRestoreServiceEnabled)) bool IsBackupRestoreServiceEnabled;
        bool get_IsBackupRestoreServiceEnabled() const { return (BackupRestoreServiceConfig::GetConfig().TargetReplicaSetSize > 0); }

        __declspec(property(get = get_ClientFactory)) Api::IClientFactoryPtr ClientFactory;
        Api::IClientFactoryPtr get_ClientFactory() const { return clientFactoryPtr_; };

        __declspec(property(get = Test_get_ClusterManagerFactory)) ClusterManager::ClusterManagerFactory & Test_ClusterManagerFactory;
        ClusterManager::ClusterManagerFactory & Test_get_ClusterManagerFactory() const;

        __declspec(property(get = get_RuntimeSharingHelper)) Hosting2::IRuntimeSharingHelper & RuntimeSharingHelper;
        Hosting2::IRuntimeSharingHelper & get_RuntimeSharingHelper() const;

        __declspec(property(get = Test_get_ActiveServices)) Test_ReplicaMap const & Test_ActiveServices;
        Test_ReplicaMap const & Test_get_ActiveServices() const;

        __declspec(property(get = get_IsUpgradeOrchestrationServiceEnabled)) bool IsUpgradeOrchestrationServiceEnabled;
        bool get_IsUpgradeOrchestrationServiceEnabled() const { return (UpgradeOrchestrationServiceConfig::GetConfig().TargetReplicaSetSize > 0); }

        __declspec(property(get = get_IsCentralSecretServiceEnabled)) bool IsCentralSecretServiceEnabled;
        bool get_IsCentralSecretServiceEnabled() const { return CentralSecretServiceConfig::GetConfig().IsCentralSecretServiceEnabled(); }

        __declspec(property(get = get_IsLocalSecretServiceEnabled)) bool IsLocalSecretServiceEnabled;
        bool get_IsLocalSecretServiceEnabled() const;

        __declspec(property(get = get_IsDnsServiceEnabled)) bool IsDnsServiceEnabled;
        bool get_IsDnsServiceEnabled() const;

        __declspec(property(get = get_IsEventStoreServiceEnabled)) bool IsEventStoreServiceEnabled;
        bool get_IsEventStoreServiceEnabled() const;
        __declspec(property(get = get_IsGatewayResourceManagerEnabled)) bool IsGatewayResourceManagerEnabled;
        bool get_IsGatewayResourceManagerEnabled() const;

        AsyncOperationSPtr BeginCreateClusterManagerService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateClusterManagerService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateRepairManagerService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateRepairManagerService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateImageStoreService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateImageStoreService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateFaultAnalysisService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateFaultAnalysisService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateBackupRestoreService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateBackupRestoreService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateUpgradeOrchestrationService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateUpgradeOrchestrationService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateCentralSecretService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateCentralSecretService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateEventStoreService(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateEventStoreService(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateSystemServiceWithReservedIdRange(
            bool isEnabled,
            ServiceDescription const & serviceDescription,
            ConsistencyUnitId::ReservedIdRange const & reservedIdRange,
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateSystemServiceWithReservedIdRange(bool isEnabled, AsyncOperationSPtr const & asyncOp);

        AsyncOperationSPtr BeginInitializeSystemServices(
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndInitializeSystemServices(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginCreateSystemServiceInstance(
            ServiceDescription const &,
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndCreateSystemServiceInstance(AsyncOperationSPtr const &);

        AsyncOperationSPtr BeginDeleteSystemServiceInstance(
            wstring const &,
            TimeSpan const,
            AsyncCallback const &,
            AsyncOperationSPtr const &);
        ErrorCode EndDeleteSystemServiceInstance(AsyncOperationSPtr const &);

        ErrorCode Open();
        ErrorCode Close();
        void Abort();

        ErrorCode SetClusterSecurity(SecuritySettings const & value);
        ErrorCode SetClientSecurity(SecuritySettings const & value);
        ErrorCode SetKeepAlive(ULONG keepAliveIntervalInSeconds);

    private:
        FederationWrapper & federation_;
        Hosting2::IRuntimeSharingHelper & runtime_;
        Hosting2::IFabricActivatorClientSPtr fabricActivatorClient_;
        ServiceAdminClient & adminClient_;
        ServiceResolver & resolver_;
        wstring clientConnectionAddress_;
        wstring clusterManagerReplicatorAddress_;
        wstring workingDir_;
        wstring nodeName_;
        Transport::SecuritySettings clusterSecuritySettings_;

        Api::IClientFactoryPtr clientFactoryPtr_;
        ComPointer<Api::ComStatefulServiceFactory> clusterManagerFactoryCPtr_;
        mutable RwLock serviceFactoryLock_;
    };

    class ManagementSubsystem::CreateSystemServiceAsyncOperation : public AsyncOperation
    {
    public:
        CreateSystemServiceAsyncOperation(
            Impl& owner,
            ServiceAdminClient & adminClient,
            ServiceDescription const & serviceDescription,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : AsyncOperation(callback, root)
            , owner_(owner)
            , adminClient_(adminClient)
            , timeout_(timeout)
            , serviceDescription_(serviceDescription)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            return AsyncOperation::End<CreateSystemServiceAsyncOperation>(operation)->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr) override
        {
            BeforeCreateService(thisSPtr);
        }

    private:
        void BeforeCreateService(AsyncOperationSPtr const & thisSPtr)
        {
            WriteInfo(
                TraceManagementSubsystem,
                "CreateSystemServiceAsyncOperation::BeforeCreateService service name {0}",
                this->serviceDescription_.Name);

            if (StringUtility::Compare(
                serviceDescription_.Name,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName) == 0)
            {
                this->BeforeCreateDnsService(thisSPtr);
            }
            else
            {
                this->OnBeforeCreateServiceCompleted(thisSPtr, ErrorCodeValue::Success);
            }
        }

        void OnBeforeCreateServiceCompleted(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
        {
            if (error.IsSuccess())
            {
                CreateService(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, error);
            }
        }

        void CreateService(AsyncOperationSPtr const & thisSPtr)
        {
            ServiceDescription versionedDescription(serviceDescription_);
            versionedDescription.Instance = DateTime::Now().Ticks;

            vector<ConsistencyUnitDescription> cuDescriptions;
            for (int ix = 0; ix < serviceDescription_.PartitionCount; ++ix)
            {
                cuDescriptions.push_back(ConsistencyUnitDescription(ConsistencyUnitId()));
            }

            auto operation = adminClient_.BeginCreateService(
                versionedDescription,
                cuDescriptions,
                FabricActivityHeader(),
                timeout_,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnCreateServiceCompleted(operation, false /*expectedCompletedSynchronously*/);
            },
                thisSPtr);
            this->OnCreateServiceCompleted(operation, true /*expectedCompletedSynchronously*/);
        }

        void OnCreateServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            CreateServiceReplyMessageBody replyBody;
            ErrorCode error = adminClient_.EndCreateService(operation, replyBody);

            if (error.IsSuccess())
            {
                error = replyBody.ErrorCodeValue;
            }

            if (error.IsError(ErrorCodeValue::FMServiceAlreadyExists))
            {
                error = ErrorCodeValue::Success;
            }

            this->TryComplete(thisSPtr, error);
        }

    private:
        //
        // DNS Service Section
        //
        void BeforeCreateDnsService(AsyncOperationSPtr const & thisSPtr)
        {
            CreateDnsNameProperty(thisSPtr);
        }

        void CreateDnsNameProperty(AsyncOperationSPtr const & thisSPtr)
        {
            ErrorCode error = owner_.ClientFactory->CreatePropertyManagementClient(propertyManagementPtr_);
            if (!error.IsSuccess()) { return; }

            const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;

            NamingUri dnsUri;
            bool success = NamingUri::TryParse(dnsName, /*out*/dnsUri);
            ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", dnsName);

            auto operation = propertyManagementPtr_->BeginCreateName(
                dnsUri,
                timeout_,
                [this](AsyncOperationSPtr const & operation) { this->OnCreateDnsNamePropertyComplete(operation, false /*expectedCompletedSynchronously*/); },
                thisSPtr);
            this->OnCreateDnsNamePropertyComplete(operation, true /*expectedCompletedSynchronously*/);
        }

        void OnCreateDnsNamePropertyComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            ErrorCode error = propertyManagementPtr_->EndCreateName(operation);

            if (error.IsError(ErrorCodeValue::NameAlreadyExists))
            {
                error = ErrorCodeValue::Success;
            }

            const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceManagementSubsystem,
                    "Failed to create DNS name property {0}, error {1}",
                    dnsName, error);
            }
            else
            {
                WriteInfo(TraceManagementSubsystem, "Successfully created DNS name property {0}", dnsName);
            }

            this->OnBeforeCreateServiceCompleted(thisSPtr, error);
        }

    private:
        Impl & owner_;
        TimeSpan timeout_;
        ServiceDescription serviceDescription_;
        ServiceAdminClient & adminClient_;
        IPropertyManagementClientPtr propertyManagementPtr_;
    };

    class ManagementSubsystem::DeleteSystemServiceAsyncOperation : public AsyncOperation
    {
    public:
        DeleteSystemServiceAsyncOperation(
            Impl& owner,
            ServiceAdminClient & adminClient,
            std::wstring const & serviceName,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : AsyncOperation(callback, root)
            , owner_(owner)
            , adminClient_(adminClient)
            , timeout_(timeout)
            , serviceName_(serviceName)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            return AsyncOperation::End<DeleteSystemServiceAsyncOperation>(operation)->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr) override
        {
            DeleteService(thisSPtr);
        }

    private:
        void DeleteService(AsyncOperationSPtr const & thisSPtr)
        {
            auto operation = this->adminClient_.BeginDeleteService(
                this->serviceName_,
                FabricActivityHeader(),
                this->timeout_,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnDeleteServiceCompleted(operation, false /*expectedCompletedSynchronously*/);
            },
                thisSPtr);

            this->OnDeleteServiceCompleted(operation, true /*expectedCompletedSynchronously*/);
        }

        void OnDeleteServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            ErrorCode error = this->adminClient_.EndDeleteService(operation);

            if (error.IsSuccess())
            {
                AfterDeleteService(thisSPtr);
            }
            else
            {
                this->TryComplete(thisSPtr, error);
            }
        }

        void AfterDeleteService(AsyncOperationSPtr const & thisSPtr)
        {
            WriteInfo(
                TraceManagementSubsystem,
                "DeleteSystemServiceAsyncOperation::AfterDeleteService service name {0}",
                this->serviceName_);

            if (StringUtility::Compare(
                this->serviceName_,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName) == 0)
            {
                this->AfterDeleteDnsService(thisSPtr);
            }
            else
            {
                this->OnAfterDeleteServiceCompleted(thisSPtr, ErrorCodeValue::Success);
            }
        }

        void OnAfterDeleteServiceCompleted(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
        {
            this->TryComplete(thisSPtr, error);
        }

    private:
        //
        // DNS Service Section
        //
        void AfterDeleteDnsService(AsyncOperationSPtr const & thisSPtr)
        {
            DeleteDnsNameProperty(thisSPtr);
        }

        void DeleteDnsNameProperty(AsyncOperationSPtr const & thisSPtr)
        {
            ErrorCode error = owner_.ClientFactory->CreatePropertyManagementClient(propertyManagementPtr_);
            if (!error.IsSuccess()) { return; }

            const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;

            NamingUri dnsUri;
            bool success = NamingUri::TryParse(dnsName, /*out*/dnsUri);
            ASSERT_IFNOT(success, "Could not parse {0} into a NamingUri", dnsName);

            auto operation = propertyManagementPtr_->BeginDeleteName(
                dnsUri,
                timeout_,
                [this](AsyncOperationSPtr const & operation) { this->OnDeleteDnsNamePropertyComplete(operation, false /*expectedCompletedSynchronously*/); },
                thisSPtr);
            this->OnDeleteDnsNamePropertyComplete(operation, true /*expectedCompletedSynchronously*/);
        }

        void OnDeleteDnsNamePropertyComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;
            const std::wstring & dnsName = ServiceModel::SystemServiceApplicationNameHelper::PublicDnsServiceName;
            ErrorCode error = this->propertyManagementPtr_->EndDeleteName(operation);

            if (error.IsSuccess())
            {
                WriteInfo(TraceManagementSubsystem, "Successfully deleted DNS name property {0}", dnsName);
            }
            else if (error.IsError(ErrorCodeValue::NameNotEmpty) || error.IsError(ErrorCodeValue::NameNotFound))
            {
                if (error.IsError(ErrorCodeValue::NameNotEmpty))
                {
                    WriteInfo(TraceManagementSubsystem, "DNS name property {0} not deleted because it is not empty.", dnsName);
                }
                error = ErrorCodeValue::Success;
            }

            if (!error.IsSuccess())
            {
                WriteWarning(TraceManagementSubsystem, "Failed to delete DNS name property {0}, error {1}", dnsName, error);
            }

            this->OnAfterDeleteServiceCompleted(thisSPtr, error);
        }

    private:
        Impl & owner_;
        TimeSpan timeout_;
        const std::wstring serviceName_;
        ServiceAdminClient & adminClient_;
        IPropertyManagementClientPtr propertyManagementPtr_;
    };

    class ManagementSubsystem::InitializeSystemServiceAsyncOperation : public AsyncOperation
    {
    private:

        using RetryCallback = std::function<void(AsyncOperationSPtr const &)>;

    public:
        InitializeSystemServiceAsyncOperation(
            TimeSpan const timeout,
            Impl & owner,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : AsyncOperation(callback, root)
            , owner_(owner)
            , timeout_(timeout)
            , retryTimerSPtr_()
            , retryTimerSPtr2_()
            , timerLock_()
            , instanceCreateIndex_(0)
            , instanceDeleteIndex_(0)
            , serviceDescriptions_()
            , instancesToCreate_()
            , instancesToDelete_()
            , queryClient_()
            , clusterManagementClient_()
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            return AsyncOperation::End<InitializeSystemServiceAsyncOperation>(operation)->Error;
        }

    protected:

        void OnStart(AsyncOperationSPtr const & thisSPtr) override
        {
            if (!ManagementConfig::GetConfig().SkipUpgradeWaitOnSystemServiceInitialization && owner_.IsCMEnabled)
            {
                this->GetClusterUpgradeStatus(thisSPtr);
            }
            else
            {
                this->ReadSystemServiceConfig(thisSPtr);
            }
        }

        void OnCancel() override
        {
            AcquireExclusiveLock lock(timerLock_);

            this->TryCancelTimer(retryTimerSPtr_);

            this->TryCancelTimer(retryTimerSPtr2_);
        }

    private:

        void GetClusterUpgradeStatus(AsyncOperationSPtr const & thisSPtr)
        {
            WriteInfo(TraceManagementSubsystem, "Start to get cluster upgrade.");

            if (clusterManagementClient_.get() == nullptr)
            {
                auto error = owner_.ClientFactory->CreateClusterManagementClient(clusterManagementClient_);
                if (!error.IsSuccess())
                {
                    WriteInfo(TraceManagementSubsystem, "Failed to create management client. error = {0}", error);
                    ScheduleRetry(
                        "Scheduling get cluster upgrade status",
                        [this](AsyncOperationSPtr const & thisSPtr) { GetClusterUpgradeStatus(thisSPtr); },
                        retryTimerSPtr_,
                        thisSPtr);
                    return;
                }
            }

            auto operation = clusterManagementClient_->BeginGetFabricUpgradeProgress(
                timeout_,
                [this](AsyncOperationSPtr const& operation) { this->OnGetClusterUpgradeStatusComplete(operation, false); },
                thisSPtr);
            this->OnGetClusterUpgradeStatusComplete(operation, true);
        }

        void OnGetClusterUpgradeStatusComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            IUpgradeProgressResultPtr upgradeResult;
            ErrorCode error = clusterManagementClient_->EndGetFabricUpgradeProgress(operation, upgradeResult);

            if (!error.IsSuccess())
            {
                WriteWarning(TraceManagementSubsystem, "Failed to get fabric upgrade progress. error = {0}", error);
                ScheduleRetry(
                    "Scheduling get cluster upgrade status",
                    [this](AsyncOperationSPtr const & thisSPtr) { GetClusterUpgradeStatus(thisSPtr); },
                    retryTimerSPtr_,
                    thisSPtr);
                return;
            }

            ClusterManager::FabricUpgradeState::Enum upgradeState = upgradeResult->GetUpgradeState();

            if (upgradeState != ClusterManager::FabricUpgradeState::Enum::CompletedRollforward && upgradeState != ClusterManager::FabricUpgradeState::Enum::CompletedRollback)
            {
                WriteInfo(TraceManagementSubsystem, "Waiting for upgrade to complete. UpgradeState: {0}", upgradeState);
                ScheduleRetry(
                    "Scheduling get cluster upgrade status",
                    [this](AsyncOperationSPtr const & thisSPtr) { GetClusterUpgradeStatus(thisSPtr); },
                    retryTimerSPtr_,
                    thisSPtr);
                return;
            }

            ReadSystemServiceConfig(thisSPtr);
        }

        void ReadSystemServiceConfig(AsyncOperationSPtr const & thisSPtr)
        {
            ManagementSubsystem::LoadAllSystemServiceDescriptions(serviceDescriptions_);

            GetSystemService(thisSPtr);
        }

        void GetSystemService(AsyncOperationSPtr const & thisSPtr)
        {
            WriteInfo(TraceManagementSubsystem, "Start to get system service.");

            if (queryClient_.get() == nullptr)
            {
                auto error = owner_.ClientFactory->CreateQueryClient(queryClient_);
                if (!error.IsSuccess())
                {
                    WriteInfo(TraceManagementSubsystem, "Failed to create query client; error = {0}", error);
                    ScheduleRetry(
                        "Scheduling get system service",
                        [this](AsyncOperationSPtr const & thisSPtr) { GetSystemService(thisSPtr); },
                        retryTimerSPtr_,
                        thisSPtr);
                    return;
                }
            }

            auto operation = queryClient_->BeginGetServiceList(
                ServiceQueryDescription(
                    NamingUri(SystemServiceApplicationNameHelper::SystemServiceApplicationAuthority),
                    NamingUri(NamingUri::RootNamingUri),
                    wstring(), /*serviceTypeNameFilter*/
                    QueryPagingDescription()), /*pagingDescription*/
                ManagementConfig::GetConfig().MaxCommunicationTimeout,
                [this](AsyncOperationSPtr const& operation) { this->OnGetSystemServiceComplete(operation, false); },
                thisSPtr);
            this->OnGetSystemServiceComplete(operation, true);
        }

        void OnGetSystemServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            PagingStatusUPtr pagingStatus;
            vector<ServiceQueryResult> serviceList;
            ErrorCode error = queryClient_->EndGetServiceList(
                operation,
                serviceList,
                pagingStatus);
            if (!error.IsSuccess())
            {
                WriteWarning(TraceManagementSubsystem, "Failed to get service list. error = {0}", error);
                ScheduleRetry(
                    "Scheduling get system service",
                    [this](AsyncOperationSPtr const & thisSPtr) { GetSystemService(thisSPtr); },
                    retryTimerSPtr_,
                    thisSPtr);
                return;
            }

            vector<wstring> serviceNamesFromConfig;
            for (auto const & description : serviceDescriptions_)
            {
                serviceNamesFromConfig.push_back(description.Name);
            }

            vector<wstring> serviceNamesFromQuery;
            vector<wstring> internalNamesFromQuery;
            for (auto const & item : serviceList)
            {
                auto serviceName = item.ServiceName.ToString();

                serviceNamesFromQuery.push_back(serviceName);

                internalNamesFromQuery.push_back(SystemServiceApplicationNameHelper::GetInternalServiceName(serviceName));
            }

            WriteInfo(
                TraceManagementSubsystem,
                "Comparing system services: config={0} query={1} internal={2}",
                serviceNamesFromConfig,
                serviceNamesFromQuery,
                internalNamesFromQuery);

            for (auto ix = 0; ix < serviceNamesFromQuery.size(); ++ix)
            {
                auto const & serviceName = serviceNamesFromQuery[ix];
                auto const & internalName = internalNamesFromQuery[ix];

                if (find(serviceNamesFromConfig.begin(), serviceNamesFromConfig.end(), internalName) == serviceNamesFromConfig.end())
                {
                    if (SystemServiceApplicationNameHelper::IsSystemServiceDeletable(internalName))
                    {
                        instancesToDelete_.push_back(internalName);
                    }
                    else
                    {
                        WriteInfo(
                            TraceManagementSubsystem,
                            "Deletion of system service {0} ({1}) is not supported",
                            serviceName,
                            internalName);
                    }
                }
            }

            for (auto const & serviceDescription : serviceDescriptions_)
            {
                if (find(internalNamesFromQuery.begin(), internalNamesFromQuery.end(), serviceDescription.Name) == internalNamesFromQuery.end())
                {
                    instancesToCreate_.push_back(serviceDescription);
                }
            }

            WriteInfo(TraceManagementSubsystem, "Completed query and comparison. Initializing Infrastructure, Token Validation, Upgrade Service");

            pendingCount_.store(static_cast<LONG>(2));
            this->CreateServiceInstance(thisSPtr);
            this->DeleteServiceInstance(thisSPtr);
        }

        __declspec(property(get = get_CurrentInstanceToCreate)) ServiceDescription const & CurrentInstanceToCreate;
        ServiceDescription const & get_CurrentInstanceToCreate() const
        {
            return instancesToCreate_[instanceCreateIndex_];
        }

        void CreateServiceInstance(AsyncOperationSPtr const & thisSPtr)
        {
            if (instanceCreateIndex_ >= instancesToCreate_.size())
            {
                // Creation of all instances completed successfully
                WriteInfo(TraceManagementSubsystem, "Completed creation of all Infrastructure, Token Validation, Upgrade Service instances");
                auto count = --pendingCount_;
                if (count < 0)
                {
                    WriteWarning(
                        TraceManagementSubsystem,
                        "Invalid create/delete pendingCount={0}",
                        count);
                    Assert::TestAssert();
                }
                if (count <= 0)
                {
                    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                }
                return;
            }

            WriteInfo(
                TraceManagementSubsystem,
                "Creating service name {0} instance {1}, replica set min = {2}, target = {3}, placement constraints = '{4}'",
                this->CurrentInstanceToCreate.Name,
                instanceCreateIndex_,
                this->CurrentInstanceToCreate.MinReplicaSetSize,
                this->CurrentInstanceToCreate.TargetReplicaSetSize,
                this->CurrentInstanceToCreate.PlacementConstraints);

            auto operation = owner_.BeginCreateSystemServiceInstance(
                this->CurrentInstanceToCreate,
                timeout_,
                [this](AsyncOperationSPtr const & operation) { this->OnCreateSystemServiceComplete(operation, false); },
                thisSPtr);
            this->OnCreateSystemServiceComplete(operation, true);
        }

        void OnCreateSystemServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            ErrorCode error = owner_.EndCreateSystemServiceInstance(operation);

            if (error.IsSuccess() || error.IsError(ErrorCodeValue::FMServiceAlreadyExists))
            {
                // Intentional no-op: Service has been created (but may not have been placed yet)
                //
                WriteInfo(TraceManagementSubsystem, "Successfully created {0} service", this->CurrentInstanceToCreate.Name);

                if (StringUtility::AreEqualCaseInsensitive(
                    this->CurrentInstanceToCreate.Name,
                    ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
                {
                    //Tracing dns service creation, This will be used in the analytics pipeline to calculate the creation of the dns service
                    dnsEventSource.DnsServiceCreated(this->CurrentInstanceToCreate.Name);
                }

                this->CompleteServiceInstanceCreation(thisSPtr);
            }
            else
            {
                WriteWarning(TraceManagementSubsystem, "Failed to create system service. error = {0}", error);
                ScheduleRetry(
                    "Scheduling create system instance",
                    [this](AsyncOperationSPtr const & thisSPtr) { CreateServiceInstance(thisSPtr); },
                    retryTimerSPtr_,
                    thisSPtr);
                return;
            }
        }

        void CompleteServiceInstanceCreation(AsyncOperationSPtr const & thisSPtr)
        {
            ++instanceCreateIndex_;

            WriteInfo(
                TraceManagementSubsystem,
                "Completed creation of system instance {0} of {1}",
                instanceCreateIndex_,
                instancesToCreate_.size());

            // Do the next instance
            this->CreateServiceInstance(thisSPtr);
        }

        __declspec(property(get = get_CurrentInstanceToDelete)) wstring const & CurrentInstanceToDelete;
        wstring const & get_CurrentInstanceToDelete() const
        {
            return instancesToDelete_[instanceDeleteIndex_];
        }

        void DeleteServiceInstance(AsyncOperationSPtr const & thisSPtr)
        {
            if (instanceDeleteIndex_ >= instancesToDelete_.size())
            {
                WriteInfo(TraceManagementSubsystem, "Completed deletion of all Infrastructure, Token Validation, Upgrade Service, EventStore Service instances");
                auto count = --pendingCount_;
                if (count < 0)
                {
                    WriteWarning(
                        TraceManagementSubsystem,
                        "Invalid create/delete pendingCount={0}",
                        count);
                    Assert::TestAssert();
                }
                if (count <= 0)
                {
                    this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                }
                return;
            }

            WriteInfo(
                TraceManagementSubsystem,
                "Deleting service name {0} instance {1}",
                this->CurrentInstanceToDelete,
                instanceDeleteIndex_);

            if (StringUtility::AreEqualCaseInsensitive(
                this->CurrentInstanceToDelete,
                ServiceModel::SystemServiceApplicationNameHelper::InternalDnsServiceName))
            {
                //Tracing dns service deletion, This will be used in the analytics pipeline to calculate the deletion of the dns service
                dnsEventSource.DnsServiceDeleted(this->CurrentInstanceToDelete);
            }

            auto operation = owner_.BeginDeleteSystemServiceInstance(
                this->CurrentInstanceToDelete,
                timeout_,
                [this](AsyncOperationSPtr const & operation) { this->OnDeleteSystemServiceComplete(operation, false); },
                thisSPtr);
            this->OnDeleteSystemServiceComplete(operation, true);
        }

        void OnDeleteSystemServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            AsyncOperationSPtr const & thisSPtr = operation->Parent;

            ErrorCode error = owner_.EndDeleteSystemServiceInstance(operation);

            if (error.IsSuccess() || error.IsError(ErrorCodeValue::FMServiceDoesNotExist))
            {
                WriteInfo(TraceManagementSubsystem, "Successfully deleted {0} service", this->CurrentInstanceToDelete);
                this->CompleteServiceInstanceDeletion(thisSPtr);
            }
            else
            {
                WriteWarning(TraceManagementSubsystem, "Failed to delete system service. error = {0}", error);
                ScheduleRetry(
                    "Scheduling delete system instance",
                    [this](AsyncOperationSPtr const & thisSPtr) { DeleteServiceInstance(thisSPtr); },
                    retryTimerSPtr2_,
                    thisSPtr);
                return;
            }
        }

        void CompleteServiceInstanceDeletion(AsyncOperationSPtr const & thisSPtr)
        {
            ++instanceDeleteIndex_;

            WriteInfo(
                TraceManagementSubsystem,
                "Completed deletion of system instance {0} of {1}",
                instanceDeleteIndex_,
                instancesToDelete_.size());

            // Do the next instance
            this->DeleteServiceInstance(thisSPtr);
        }

        void ScheduleRetry(
            StringLiteral const & msg,
            RetryCallback const & callback,
            TimerSPtr & retryTimerSPtr,
            AsyncOperationSPtr const & thisSPtr)
        {
            {
                AcquireExclusiveLock lock(timerLock_);

                if (!this->IsCancelRequested)
                {
                    this->TryCancelTimer(retryTimerSPtr);

                    retryTimerSPtr = Timer::Create(
                        TimerTagDefault,
                        [this, thisSPtr, callback](TimerSPtr const& timer)
                    {
                        timer->Cancel();
                        callback(thisSPtr);
                    });
                }
            }

            if (retryTimerSPtr.get() != nullptr)
            {
                WriteInfo(TraceManagementSubsystem, msg);

                retryTimerSPtr->Change(timeout_);
            }
        }

        _Requires_lock_held_(timerLock_)
            void TryCancelTimer(TimerSPtr const& retryTimerSPtr)
        {
            if (retryTimerSPtr.get() != nullptr)
            {
                retryTimerSPtr->Cancel();
            }
        }

        Impl & owner_;
        TimeSpan timeout_;
        TimerSPtr retryTimerSPtr_;
        TimerSPtr retryTimerSPtr2_;
        ExclusiveLock timerLock_;
        vector<ServiceDescription> serviceDescriptions_;
        vector<ServiceDescription> instancesToCreate_;
        vector<wstring> instancesToDelete_;
        int instanceCreateIndex_;
        int instanceDeleteIndex_;

        Common::atomic_long pendingCount_;

        IQueryClientPtr queryClient_;
        IClusterManagementClientPtr clusterManagementClient_;
        DnsEventSource const dnsEventSource;
    };

    // ************
    // private impl
    // ************

    ManagementSubsystem::Impl::Impl(
        FederationWrapper & federation,
        Hosting2::IRuntimeSharingHelper & runtime,
        Hosting2::IFabricActivatorClientSPtr const & fabricActivatorClient,
        ServiceAdminClient & adminClient,
        ServiceResolver & resolver,
        wstring const & clientConnectionAddress,
        IClientFactoryPtr &&clientFactory,
        wstring const & clusterManagerReplicatorAddress,
        wstring const & workingDir,
        wstring const & nodeName,
        Transport::SecuritySettings const& clusterSecuritySettings,
        ComponentRoot const & root)
        : RootedObject(root)
        , federation_(federation)
        , runtime_(runtime)
        , fabricActivatorClient_(fabricActivatorClient)
        , adminClient_(adminClient)
        , resolver_(resolver)
        , clientConnectionAddress_(clientConnectionAddress)
        , clusterManagerReplicatorAddress_(clusterManagerReplicatorAddress)
        , workingDir_(workingDir)
        , nodeName_(nodeName)
        , clusterSecuritySettings_(clusterSecuritySettings)
        , clientFactoryPtr_(move(clientFactory))
        , clusterManagerFactoryCPtr_()
        , serviceFactoryLock_()
    {
    }

    bool ManagementSubsystem::Impl::get_IsISEnabled() const
    {
        vector<ServiceDescription> serviceDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            serviceDescriptions,
            SystemServiceApplicationNameHelper::InternalInfrastructureServiceName);
        return (!serviceDescriptions.empty());
    }

    bool ManagementSubsystem::Impl::get_IsTVSEnabled() const
    {
        vector<ServiceDescription> dstsDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            dstsDescriptions,
            SystemServiceApplicationNameHelper::InternalDSTSTokenValidationServiceName);

        vector<ServiceDescription> tvsDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            tvsDescriptions,
            SystemServiceApplicationNameHelper::InternalTokenValidationServiceName);

        return (!dstsDescriptions.empty() || !tvsDescriptions.empty());
    }

    bool ManagementSubsystem::Impl::get_IsImageStoreServiceEnabled() const
    {
        return (this->IsCMEnabled
            && (StringUtility::StartsWith<wstring>(ManagementConfig::GetConfig().ImageStoreConnectionString.GetPlaintext(), *Management::ImageStore::Constants::NativeImageStoreSchemaTag)
                || Management::ImageStore::ImageStoreServiceConfig::GetConfig().Enabled)
            && ImageStoreServiceConfig::GetConfig().TargetReplicaSetSize > 0);
    }

    bool ManagementSubsystem::Impl::get_IsUpgradeServiceEnabled() const
    {
        vector<ServiceDescription> serviceDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            serviceDescriptions,
            SystemServiceApplicationNameHelper::InternalUpgradeServiceName);
        return (!serviceDescriptions.empty());
    }

    bool ManagementSubsystem::Impl::get_IsDnsServiceEnabled() const
    {
        vector<ServiceDescription> dnsServiceDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            dnsServiceDescriptions,
            SystemServiceApplicationNameHelper::InternalDnsServiceName);

        return (!dnsServiceDescriptions.empty());
    }

    bool ManagementSubsystem::Impl::get_IsLocalSecretServiceEnabled() const
    {
        vector<ServiceDescription> serviceDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            serviceDescriptions,
            SystemServiceApplicationNameHelper::InternalLocalSecretServiceName);

        return (!serviceDescriptions.empty());
    }

    bool ManagementSubsystem::Impl::get_IsEventStoreServiceEnabled() const
    {
        EventStoreServiceConfig serviceConfig;
        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(
            *SystemServiceApplicationNameHelper::InternalEventStoreServiceName,
            configSection);

        return serviceConfig.TryReadFromConfig(SystemServiceApplicationNameHelper::InternalEventStoreServiceName, configSection);
    }

    bool ManagementSubsystem::Impl::get_IsGatewayResourceManagerEnabled() const
    {
        vector<ServiceDescription> serviceDescriptions;
        ManagementSubsystem::LoadSystemServiceDescriptions(
            serviceDescriptions,
            SystemServiceApplicationNameHelper::InternalGatewayResourceManagerName);
        return (!serviceDescriptions.empty());
    }

    ClusterManager::ClusterManagerFactory & ManagementSubsystem::Impl::Test_get_ClusterManagerFactory() const
    {
        ComPointer<Api::ComStatefulServiceFactory> snap;
        {
            AcquireReadLock lock(serviceFactoryLock_);

            snap = clusterManagerFactoryCPtr_;
        }

        if (!snap)
        {
            Assert::CodingError("Test_ClusterManagerFactory: ManagementSubsystem has been closed");
        }

        return *dynamic_cast<ClusterManager::ClusterManagerFactory*>(snap->get_Impl().get());
    }

    Hosting2::IRuntimeSharingHelper & ManagementSubsystem::Impl::get_RuntimeSharingHelper() const
    {
        return runtime_;
    }

    Test_ReplicaMap const & ManagementSubsystem::Impl::Test_get_ActiveServices() const
    {
        return this->Test_ClusterManagerFactory.Test_ActiveServices;
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateSystemServiceWithReservedIdRange(
        bool isEnabled,
        ServiceDescription const & serviceDescription,
        ConsistencyUnitId::ReservedIdRange const & reservedIdRange,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        if (isEnabled)
        {
            auto serviceDescriptionWithInstance = serviceDescription;
            serviceDescriptionWithInstance.Instance = DateTime::Now().Ticks;

            vector<ConsistencyUnitDescription> cuDescriptions;
            for (int ix = 0; ix < serviceDescription.PartitionCount; ++ix)
            {
                cuDescriptions.push_back(ConsistencyUnitDescription(ConsistencyUnitId::CreateReserved(reservedIdRange, ix)));
            }

            return adminClient_.BeginCreateService(
                serviceDescriptionWithInstance,
                cuDescriptions,
                FabricActivityHeader(),
                timeout,
                callback,
                parent);
        }
        else
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, parent);
        }
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateSystemServiceWithReservedIdRange(bool isEnabled, AsyncOperationSPtr const & operation)
    {
        if (isEnabled)
        {
            CreateServiceReplyMessageBody replyBody;
            ErrorCode error = adminClient_.EndCreateService(operation, replyBody);

            if (error.IsSuccess())
            {
                error = replyBody.ErrorCodeValue;
            }

            return error;
        }
        else
        {
            return CompletedAsyncOperation::End(operation);
        }
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateClusterManagerService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsCMEnabled,
            CreateClusterManagerServiceDescription(),
            *ConsistencyUnitId::CMIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateClusterManagerService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsCMEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateRepairManagerService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsRMEnabled,
            CreateRepairManagerServiceDescription(),
            *ConsistencyUnitId::RMIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateRepairManagerService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsRMEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateImageStoreService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsImageStoreServiceEnabled,
            CreateImageStoreServiceDescription(),
            *ConsistencyUnitId::FileStoreIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateImageStoreService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsImageStoreServiceEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateFaultAnalysisService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsFaultAnalysisServiceEnabled,
            CreateFaultAnalysisServiceDescription(),
            *ConsistencyUnitId::FaultAnalysisServiceIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateFaultAnalysisService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsFaultAnalysisServiceEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateBackupRestoreService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsBackupRestoreServiceEnabled,
            CreateBackupRestoreServiceDescription(),
            *ConsistencyUnitId::BackupRestoreServiceIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateBackupRestoreService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsBackupRestoreServiceEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginInitializeSystemServices(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto operation = AsyncOperation::CreateAndStart<InitializeSystemServiceAsyncOperation>(
            timeout,
            *this,
            callback,
            parent);

        return operation;
    }

    ErrorCode ManagementSubsystem::Impl::EndInitializeSystemServices(AsyncOperationSPtr const & operation)
    {
        return InitializeSystemServiceAsyncOperation::End(operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateCentralSecretService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsCentralSecretServiceEnabled,
            CreateCentralSecretServiceDescription(),
            *ConsistencyUnitId::CentralSecretServiceIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateCentralSecretService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsCentralSecretServiceEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateUpgradeOrchestrationService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsUpgradeOrchestrationServiceEnabled,
            CreateUpgradeOrchestrationServiceDescription(),
            *ConsistencyUnitId::UpgradeOrchestrationServiceIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateUpgradeOrchestrationService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsUpgradeOrchestrationServiceEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateEventStoreService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateSystemServiceWithReservedIdRange(
            IsEventStoreServiceEnabled,
            CreateEventStoreServiceDescription(),
            *ConsistencyUnitId::EventStoreServiceIdRange,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateEventStoreService(AsyncOperationSPtr const & operation)
    {
        return EndCreateSystemServiceWithReservedIdRange(IsEventStoreServiceEnabled, operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginCreateSystemServiceInstance(
        ServiceDescription const & serviceDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CreateSystemServiceAsyncOperation>(
            *this,
            adminClient_,
            serviceDescription,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndCreateSystemServiceInstance(AsyncOperationSPtr const & operation)
    {
        return CreateSystemServiceAsyncOperation::End(operation);
    }

    AsyncOperationSPtr ManagementSubsystem::Impl::BeginDeleteSystemServiceInstance(
        wstring const & serviceName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<DeleteSystemServiceAsyncOperation>(
            *this,
            adminClient_,
            serviceName,
            timeout,
            callback,
            parent);
    }

    ErrorCode ManagementSubsystem::Impl::EndDeleteSystemServiceInstance(AsyncOperationSPtr const & operation)
    {
        return DeleteSystemServiceAsyncOperation::End(operation);
    }

    ErrorCode ManagementSubsystem::Impl::Open()
    {
        if (!this->IsEnabled)
        {
            return ErrorCodeValue::Success;
        }

        ComPointer<IFabricRuntime> fabricRuntimeCPtr = runtime_.GetRuntime();
        if (!fabricRuntimeCPtr)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        IClientSettingsPtr settingsPtr;
        auto error = clientFactoryPtr_->CreateSettingsClient(settingsPtr);
        if (!error.IsSuccess()) { return error; }

        if (this->IsCMEnabled)
        {
            wstring imageBuilderPath = workingDir_;
            if (ManagementConfig::GetConfig().EnableDeploymentAtDataRoot)
            {
                error = FabricEnvironment::GetFabricDataRoot(imageBuilderPath);
                if (!error.IsSuccess())
                {
                    return error;
                }
            }
            else
            {
                auto deploymentDir = ManagementConfig::GetConfig().DeploymentDirectory;
                if (!deploymentDir.empty() &&
                    Path::IsPathRooted(deploymentDir))
                {
                    imageBuilderPath = deploymentDir;
                }
            }
            auto factorySPtr = shared_ptr<ClusterManager::ClusterManagerFactory>(new ClusterManagerFactory(
                federation_,
                resolver_,
                clientConnectionAddress_,
                clientFactoryPtr_,
                clusterManagerReplicatorAddress_,
                workingDir_,
                imageBuilderPath,
                nodeName_,
                clusterSecuritySettings_,
                this->Root));

            ComPointer<Api::ComStatefulServiceFactory> snap;
            {
                AcquireWriteLock lock(serviceFactoryLock_);

                clusterManagerFactoryCPtr_.SetNoAddRef(new Api::ComStatefulServiceFactory(IStatefulServiceFactoryPtr(
                    factorySPtr.get(),
                    factorySPtr->CreateComponentRoot())));

                snap = clusterManagerFactoryCPtr_;
            }

            auto hr = fabricRuntimeCPtr->RegisterStatefulServiceFactory(
                ServiceModel::ServiceTypeIdentifier::ClusterManagerServiceTypeId->ServiceTypeName.c_str(),
                snap.GetRawPointer());

            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode ManagementSubsystem::Impl::Close()
    {
        if (!this->IsEnabled) { return ErrorCodeValue::Success; }

        ComPointer<Api::ComStatefulServiceFactory> snap;
        {
            AcquireWriteLock lock(serviceFactoryLock_);

            snap.Swap(clusterManagerFactoryCPtr_);
        }

        return ErrorCodeValue::Success;
    }

    void ManagementSubsystem::Impl::Abort()
    {
        this->Close().ReadValue();
    }

    ErrorCode ManagementSubsystem::Impl::SetClusterSecurity(SecuritySettings const & value)
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (this->IsCMEnabled)
        {
            ComPointer<Api::ComStatefulServiceFactory> snap;
            {
                AcquireReadLock lock(serviceFactoryLock_);

                snap = clusterManagerFactoryCPtr_;
            }

            if (!snap)
            {
                return ErrorCodeValue::ObjectClosed;
            }

            error = dynamic_cast<ClusterManager::ClusterManagerFactory*>(snap->get_Impl().get())->UpdateReplicatorSecuritySettings(value);
        }

        return error;
    }

    ErrorCode ManagementSubsystem::Impl::SetKeepAlive(ULONG keepAliveIntervalInSeconds)
    {
        if (!this->IsEnabled) { return ErrorCodeValue::Success; }

        IClientSettingsPtr settingsPtr;
        auto error = clientFactoryPtr_->CreateSettingsClient(settingsPtr);
        if (!error.IsSuccess()) { return error; }

        error = settingsPtr->SetKeepAlive(keepAliveIntervalInSeconds);
        return error;
    }

    // ****************************
    // public class -> private impl
    // ****************************

    ManagementSubsystem::ManagementSubsystem(
        FederationWrapper & federation,
        Hosting2::IRuntimeSharingHelper & runtime,
        Hosting2::IFabricActivatorClientSPtr const & fabricActivatorClient,
        ServiceAdminClient & adminClient,
        ServiceResolver & resolver,
        wstring const & clientConnectionAddress,
        IClientFactoryPtr &&clientFactory,
        wstring const & clusterManagerReplicatorAddress,
        wstring const & workingDir,
        wstring const & nodeName,
        Transport::SecuritySettings const& clusterSecuritySettings,
        ComponentRoot const & root)
        : RootedObject(root)
        , impl_(make_unique<Impl>(
            federation,
            runtime,
            fabricActivatorClient,
            adminClient,
            resolver,
            clientConnectionAddress,
            move(clientFactory),
            clusterManagerReplicatorAddress,
            workingDir,
            nodeName,
            clusterSecuritySettings,
            root))
    {
        REGISTER_MESSAGE_HEADER(CreateComposeDeploymentRequestHeader);
    }

    ManagementSubsystem::~ManagementSubsystem()
    {
    }

    bool ManagementSubsystem::get_IsEnabled() const { return impl_->IsEnabled; }

    bool ManagementSubsystem::get_IsCMEnabled() const { return impl_->IsCMEnabled; }

    bool ManagementSubsystem::get_IsRMEnabled() const { return impl_->IsRMEnabled; }

    bool ManagementSubsystem::get_IsISEnabled() const { return impl_->IsISEnabled; }

    bool ManagementSubsystem::get_IsTVSEnabled() const { return impl_->IsTVSEnabled; }

    bool ManagementSubsystem::get_IsImageStoreServiceEnabled() const { return impl_->IsImageStoreServiceEnabled; }

    bool ManagementSubsystem::get_IsUpgradeServiceEnabled() const { return impl_->IsUpgradeServiceEnabled; }

    bool ManagementSubsystem::get_IsFaultAnalysisServiceEnabled() const { return impl_->IsFaultAnalysisServiceEnabled; }

    bool ManagementSubsystem::get_IsBackupRestoreServiceEnabled() const { return impl_->IsBackupRestoreServiceEnabled; }

    bool ManagementSubsystem::get_IsCentralSecretServiceEnabled() const { return impl_->IsCentralSecretServiceEnabled; }
   
    bool ManagementSubsystem::get_IsUpgradeOrchestrationServiceEnabled() const { return impl_->IsUpgradeOrchestrationServiceEnabled; }

    bool ManagementSubsystem::get_IsDnsServiceEnabled() const { return impl_->IsDnsServiceEnabled; }

    bool ManagementSubsystem::get_IsLocalSecretServiceEnabled() const { return impl_->IsLocalSecretServiceEnabled; }

    bool ManagementSubsystem::get_IsEventStoreServiceEnabled() const { return impl_->IsEventStoreServiceEnabled; }
    bool ManagementSubsystem::get_IsGatewayResourceManagerEnabled() const { return impl_->IsGatewayResourceManagerEnabled; }

    ClusterManager::ClusterManagerFactory & ManagementSubsystem::Test_get_ClusterManagerFactory() const
    {
        return impl_->Test_ClusterManagerFactory;
    }

    Hosting2::IRuntimeSharingHelper & ManagementSubsystem::get_RuntimeSharingHelper() const
    {
        return impl_->RuntimeSharingHelper;
    }

    Test_ReplicaMap const & ManagementSubsystem::Test_get_ActiveServices() const
    {
        return impl_->Test_ActiveServices;
    }

    //
    // Cluster Manager Service
    //

    AsyncOperationSPtr ManagementSubsystem::BeginCreateClusterManagerService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateClusterManagerService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateClusterManagerService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateClusterManagerService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateClusterManagerServiceDescription()
    {
        return CreateClusterManagerServiceDescription(
            ManagementConfig::GetConfig().TargetReplicaSetSize,
            ManagementConfig::GetConfig().MinReplicaSetSize,
            ManagementConfig::GetConfig().ReplicaRestartWaitDuration,
            ManagementConfig::GetConfig().QuorumLossWaitDuration,
            ManagementConfig::GetConfig().StandByReplicaKeepDuration,
            ManagementConfig::GetConfig().PlacementConstraints);
    }

    ServiceDescription ManagementSubsystem::CreateClusterManagerServiceDescription(
        int targetReplicaSetSize,
        int minReplicaSetSize,
        TimeSpan replicaRestartWaitDuration,
        TimeSpan quorumLossWaitDuration,
        TimeSpan standByReplicaKeepDuration,
        wstring const& placementConstraints)
    {
        // ensure this service is isolated to other services
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            ClusterManager::Constants::ClusterManagerServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            ClusterManager::Constants::ClusterManagerServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            targetReplicaSetSize,
            minReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            replicaRestartWaitDuration,
            quorumLossWaitDuration,
            standByReplicaKeepDuration,
            *ServiceTypeIdentifier::ClusterManagerServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            placementConstraints, // placement constraints
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            vector<byte>());
    }

    //
    // Repair Manager Service
    //

    AsyncOperationSPtr ManagementSubsystem::BeginCreateRepairManagerService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateRepairManagerService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateRepairManagerService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateRepairManagerService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateRepairManagerServiceDescription()
    {
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            RepairManager::Constants::RepairManagerServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            RepairManager::Constants::RepairManagerServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        RepairManagerConfig const & config = RepairManagerConfig::GetConfig();

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalRepairManagerServiceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            config.TargetReplicaSetSize,
            config.MinReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            config.ReplicaRestartWaitDuration,
            config.QuorumLossWaitDuration,
            config.StandByReplicaKeepDuration,
            *ServiceTypeIdentifier::RepairManagerServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    //
    // Image Store Service
    //

    AsyncOperationSPtr ManagementSubsystem::BeginCreateImageStoreService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateImageStoreService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateImageStoreService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateImageStoreService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateImageStoreServiceDescription()
    {
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            FileStoreService::Constants::FileStoreServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            FileStoreService::Constants::FileStoreServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        vector<Reliability::ServiceCorrelationDescription> serviceCorrelationDescList;
        if (ImageStoreServiceConfig::GetConfig().EnableClusterManagerAffinity)
        {
            // Co-locate CM and FileStoreService primaries to optimize application package transfers
            //
            Reliability::ServiceCorrelationDescription serviceCorrelationDesc(
                ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName,
                FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY);
            serviceCorrelationDescList.push_back(serviceCorrelationDesc);
        }

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalImageStoreServiceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            ImageStoreServiceConfig::GetConfig().TargetReplicaSetSize,
            ImageStoreServiceConfig::GetConfig().MinReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            ImageStoreServiceConfig::GetConfig().ReplicaRestartWaitDuration,
            ImageStoreServiceConfig::GetConfig().QuorumLossWaitDuration,
            ImageStoreServiceConfig::GetConfig().StandByReplicaKeepDuration,
            *ServiceTypeIdentifier::FileStoreServiceTypeId,
            serviceCorrelationDescList,
            ImageStoreServiceConfig::GetConfig().PlacementConstraints, // placement constraints
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    //
    // Infrastructure Service
    //

    AsyncOperationSPtr ManagementSubsystem::BeginInitializeSystemServices(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginInitializeSystemServices(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndInitializeSystemServices(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndInitializeSystemServices(asyncOperation);
    }

    void ManagementSubsystem::LoadAllSystemServiceDescriptions(vector<ServiceDescription> & descriptions)
    {
        LoadSystemServiceDescriptions(descriptions, L"");
    }

    void ManagementSubsystem::LoadSystemServiceDescriptions(
        vector<ServiceDescription> & descriptions,
        wstring const & nameFilter)
    {
        StringCollection sectionNames;
        ManagementConfig::GetConfig().GetSections(sectionNames);

        for (auto const & sectionName : sectionNames)
        {
            if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(nameFilter, SystemServiceApplicationNameHelper::InternalInfrastructureServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalInfrastructureServiceName) ||
                    StringUtility::StartsWithCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalInfrastructureServicePrefix)))
            {
                AddInfrastructureServiceDescription(sectionName, descriptions);
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(nameFilter, SystemServiceApplicationNameHelper::InternalDSTSTokenValidationServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalDSTSTokenValidationServiceName) ||
                    StringUtility::StartsWithCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalDSTSTokenValidationServicePrefix)))
            {
                AddTokenValidationServiceDescription(sectionName, descriptions);
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(nameFilter, SystemServiceApplicationNameHelper::InternalTokenValidationServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalTokenValidationServiceName) ||
                    StringUtility::StartsWithCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalTokenValidationServicePrefix)))
            {
                AddTokenValidationServiceDescription(sectionName, descriptions);
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(nameFilter, SystemServiceApplicationNameHelper::InternalUpgradeServiceName)) &&
                StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalUpgradeServiceName))
            {
                descriptions.push_back(CreateUpgradeServiceDescription(sectionName));
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(sectionName, SystemServiceApplicationNameHelper::InternalDnsServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalDnsServiceName)))
            {
                AddDnsServiceDescription(sectionName, descriptions);
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(sectionName, SystemServiceApplicationNameHelper::InternalLocalSecretServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalLocalSecretServiceName)))
            {
                AddLocalSecretServiceDescription(sectionName, descriptions);
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(sectionName, SystemServiceApplicationNameHelper::InternalBackupRestoreServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalBackupRestoreServiceName)))
            {
                descriptions.push_back(CreateBackupRestoreServiceDescription());
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(sectionName, SystemServiceApplicationNameHelper::InternalResourceMonitorServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalResourceMonitorServiceName)))
            {
                AddResourceMonitorServiceDescription(sectionName, descriptions);
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(nameFilter, SystemServiceApplicationNameHelper::InternalEventStoreServiceName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalEventStoreServiceName)))
            {
                descriptions.push_back(CreateEventStoreServiceDescription());
            }
            else if ((nameFilter.empty() || StringUtility::AreEqualCaseInsensitive(nameFilter, SystemServiceApplicationNameHelper::InternalGatewayResourceManagerName)) &&
                (StringUtility::AreEqualCaseInsensitive(sectionName, *SystemServiceApplicationNameHelper::InternalGatewayResourceManagerName)))
            {
                AddGatewayResourceManagerServiceDescription(sectionName, descriptions);
            }
        }
    }

    void ManagementSubsystem::AddInfrastructureServiceDescription(wstring const & serviceName, __in vector<ServiceDescription> & descriptions)
    {
        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(serviceName, configSection);

        InfrastructureServiceConfig serviceConfig;
        if (!serviceConfig.TryReadFromConfig(serviceName, configSection))
        {
            WriteWarning(
                TraceManagementSubsystem,
                "Contents of configuration section '{0}' are invalid; ignoring this instance",
                serviceName);

            return;
        }

        if ((serviceConfig.MinReplicaSetSize <= 0) || (serviceConfig.MinReplicaSetSize > serviceConfig.TargetReplicaSetSize))
        {
            WriteInfo(
                TraceManagementSubsystem,
                "Configuration section '{0}' specifies replica set min = {1}, target = {2}; ignoring this instance",
                serviceName,
                serviceConfig.MinReplicaSetSize,
                serviceConfig.TargetReplicaSetSize);

            return;
        }

        // ensure this service is isolated to other services
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            ServiceModel::Constants::InfrastructureServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            ServiceModel::Constants::InfrastructureServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceConfig.SectionName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceConfig.SectionName.size() * sizeof(wchar_t));

        descriptions.push_back(ServiceDescription(
            serviceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            serviceConfig.TargetReplicaSetSize,
            serviceConfig.MinReplicaSetSize,
            true,   // is stateful
            false,   // has persisted state
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::InfrastructureServicePackageName),
                SystemServiceApplicationNameHelper::InfrastructureServiceType),
            vector<Reliability::ServiceCorrelationDescription>(),
            serviceConfig.PlacementConstraints, // placement constraints
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            move(configSectionNameSerialized),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName));
    }

    void ManagementSubsystem::AddTokenValidationServiceDescription(wstring const & serviceName, __in vector<ServiceDescription> & descriptions)
    {
        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(serviceName, configSection);

        TokenValidationServiceConfig serviceConfig;
        if (!serviceConfig.TryReadFromConfig(serviceName, configSection))
        {
            WriteWarning(
                TraceManagementSubsystem,
                "Contents of configuration section '{0}' are invalid; ignoring this instance",
                serviceName);

            return;
        }

        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceName.size() * sizeof(wchar_t));

        descriptions.push_back(ServiceDescription(
            serviceName,
            0, // instance
            0, // update version
            1, // partition count
            -1, // target replica set size
            -1, // min replica set size
            false, // stateful
            false, // persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::TokenValidationServicePackageName),
                SystemServiceApplicationNameHelper::TokenValidationServiceType),
            vector<Reliability::ServiceCorrelationDescription>(),
            serviceConfig.PlacementConstraints,
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            configSectionNameSerialized,
            SystemServiceApplicationNameHelper::SystemServiceApplicationName));
    }

    void ManagementSubsystem::AddDnsServiceDescription(wstring const & serviceName, __in vector<ServiceDescription> & descriptions)
    {
        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(serviceName, configSection);

        DnsServiceConfig const & config = DnsServiceConfig::GetConfig();

        if (!config.IsEnabled)
        {
            return;
        }

        WriteInfo(TraceManagementSubsystem, "DnsService configuration: instance count is {0} ", config.InstanceCount);

        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceName.size() * sizeof(wchar_t));

        descriptions.push_back(ServiceDescription(
            serviceName,
            0, // instance
            0, // update version
            1, // partition count
            config.InstanceCount, // target replica set size
            config.InstanceCount, // min replica set size
            false, // stateful
            false, // persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::DnsServicePackageName),
                SystemServiceApplicationNameHelper::DnsServiceType),
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            configSectionNameSerialized,
            SystemServiceApplicationNameHelper::SystemServiceApplicationName));
    }

    void ManagementSubsystem::AddResourceMonitorServiceDescription(wstring const & serviceName, __in vector<ServiceDescription> & descriptions)
    {
        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(serviceName, configSection);

        ResourceMonitorServiceConfig const & config = ResourceMonitorServiceConfig::GetConfig();

        if (!config.IsEnabled)
        {
            return;
        }

        WriteInfo(TraceManagementSubsystem, "ResourceMonitorService configuration: instance count is {0} ", config.InstanceCount);

        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceName.size() * sizeof(wchar_t));

        descriptions.push_back(ServiceDescription(
            serviceName,
            0, // instance
            0, // update version
            1, // partition count
            config.InstanceCount, // target replica set size
            config.InstanceCount, // min replica set size
            false, // stateful
            false, // persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::ResourceMonitorServicePackageName),
                SystemServiceApplicationNameHelper::ResourceMonitorServiceType),
            vector<Reliability::ServiceCorrelationDescription>(),
            L"",
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            configSectionNameSerialized,
            SystemServiceApplicationNameHelper::SystemServiceApplicationName));
    }

    void ManagementSubsystem::AddLocalSecretServiceDescription(wstring const & serviceName, __in vector<ServiceDescription> & descriptions)
    {
        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(serviceName, configSection);

        LocalSecretServiceConfig const & config = LocalSecretServiceConfig::GetConfig();

        WriteInfo(TraceManagementSubsystem, "LocalSecretService configuration: instance count is {0} ", config.InstanceCount);

        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceName.size() * sizeof(wchar_t));

        descriptions.push_back(ServiceDescription(
            serviceName,
            0, // instance
            0, // update version
            1, // partition count
            config.InstanceCount, // target replica set size
            config.InstanceCount, // min replica set size
            false, // stateful
            false, // persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::LocalSecretServicePackageName),
                SystemServiceApplicationNameHelper::LocalSecretServiceType),
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            configSectionNameSerialized,
            SystemServiceApplicationNameHelper::SystemServiceApplicationName));
    }

    void ManagementSubsystem::AddGatewayResourceManagerServiceDescription(wstring const & serviceName, __in vector<ServiceDescription> & descriptions)
    {
        GatewayResourceManagerConfig const & serviceConfig = GatewayResourceManagerConfig::GetConfig();
        if ((serviceConfig.MinReplicaSetSize <= 0) || (serviceConfig.MinReplicaSetSize > serviceConfig.TargetReplicaSetSize))
        {
            WriteInfo(
                TraceManagementSubsystem,
                "Configuration section '{0}' specifies replica set min = {1}, target = {2}; ignoring this instance",
                serviceName,
                serviceConfig.MinReplicaSetSize,
                serviceConfig.TargetReplicaSetSize);

            return;
        }

        // ensure this service is isolated to other services
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            GatewayResourceManager::Constants::GatewayResourceManagerPrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            GatewayResourceManager::Constants::GatewayResourceManagerReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceName.size() * sizeof(wchar_t));

        descriptions.push_back(ServiceDescription(
            serviceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            serviceConfig.TargetReplicaSetSize,
            serviceConfig.MinReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::GatewayResourceManagerPackageName),
                SystemServiceApplicationNameHelper::GatewayResourceManagerType),
            vector<Reliability::ServiceCorrelationDescription>(),
            serviceConfig.PlacementConstraints, // placement constraints
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            move(configSectionNameSerialized),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName));
    }

    ServiceDescription ManagementSubsystem::CreateUpgradeServiceDescription(wstring const & serviceName)
    {
        // Provide the config section name for this service instance as initialization data to the service,
        // so that the managed service code can read additional settings from the same section later.
        const byte *configSectionNameBuffer = reinterpret_cast<const byte *>(serviceName.c_str());
        vector<byte> configSectionNameSerialized(
            configSectionNameBuffer,
            configSectionNameBuffer + serviceName.size() * sizeof(wchar_t));

        return ServiceDescription(
            serviceName,
            0, //instance
            0, //update version
            1, //partition count
            UpgradeServiceConfig::GetConfig().TargetReplicaSetSize, // Target replica size
            UpgradeServiceConfig::GetConfig().MinReplicaSetSize, // Min replica size
            true,//Is stateful
            true, // Is persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            ServiceTypeIdentifier(
                ServicePackageIdentifier(
                    ApplicationIdentifier::FabricSystemAppId->ToString(),
                    SystemServiceApplicationNameHelper::UpgradeServicePackageName),
                SystemServiceApplicationNameHelper::UpgradeServiceType),
            vector<Reliability::ServiceCorrelationDescription>(),
            UpgradeServiceConfig::GetConfig().PlacementConstraints, // placement constraints
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            configSectionNameSerialized,
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    ServiceDescription ManagementSubsystem::CreateBackupRestoreServiceDescription()
    {
        BackupRestoreServiceConfig const & config = BackupRestoreServiceConfig::GetConfig();

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalBackupRestoreServiceName,
            0, // instance
            0, // update version
            1, // partition count
            config.TargetReplicaSetSize, // Target replica size
            config.MinReplicaSetSize, // Min replica size
            true, // Is stateful
            true, // Is persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            *ServiceTypeIdentifier::BackupRestoreServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints, // placement constraints
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    AsyncOperationSPtr ManagementSubsystem::BeginCreateBackupRestoreService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateBackupRestoreService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateBackupRestoreService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateBackupRestoreService(asyncOperation);
    }

    AsyncOperationSPtr ManagementSubsystem::BeginCreateFaultAnalysisService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateFaultAnalysisService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateFaultAnalysisService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateFaultAnalysisService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateFaultAnalysisServiceDescription()
    {
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            FaultAnalysisService::Constants::FaultAnalysisServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            FaultAnalysisService::Constants::FaultAnalysisServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        FaultAnalysisServiceConfig const & config = FaultAnalysisServiceConfig::GetConfig();

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalFaultAnalysisServiceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            config.TargetReplicaSetSize,
            config.MinReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            config.ReplicaRestartWaitDuration,
            config.QuorumLossWaitDuration,
            config.StandByReplicaKeepDuration,
            *ServiceTypeIdentifier::FaultAnalysisServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    AsyncOperationSPtr ManagementSubsystem::BeginCreateCentralSecretService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateCentralSecretService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateCentralSecretService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateCentralSecretService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateCentralSecretServiceDescription()
    {
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            CentralSecretService::Constants::CentralSecretServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            CentralSecretService::Constants::CentralSecretServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        CentralSecretServiceConfig const & config = CentralSecretServiceConfig::GetConfig();

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalCentralSecretServiceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            config.TargetReplicaSetSize,
            config.MinReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            config.ReplicaRestartWaitDuration,
            config.QuorumLossWaitDuration,
            config.StandByReplicaKeepDuration,
            *ServiceTypeIdentifier::CentralSecretServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    AsyncOperationSPtr ManagementSubsystem::BeginCreateUpgradeOrchestrationService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateUpgradeOrchestrationService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateUpgradeOrchestrationService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateUpgradeOrchestrationService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateUpgradeOrchestrationServiceDescription()
    {
        vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            UpgradeOrchestrationService::Constants::UpgradeOrchestrationServicePrimaryCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
            1,
            0));
        serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
            UpgradeOrchestrationService::Constants::UpgradeOrchestrationServiceReplicaCountName,
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
            1,
            1));

        UpgradeOrchestrationServiceConfig const & config = UpgradeOrchestrationServiceConfig::GetConfig();

        return ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalUpgradeOrchestrationServiceName,
            0,  // instance
            0,  // update version
            1,  // partition count
            config.TargetReplicaSetSize,
            config.MinReplicaSetSize,
            true,   // is stateful
            true,   // has persisted state
            config.ReplicaRestartWaitDuration,
            config.QuorumLossWaitDuration,
            config.StandByReplicaKeepDuration,
            *ServiceTypeIdentifier::UpgradeOrchestrationServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0, // scaleout count
            move(serviceMetricDescriptions),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    AsyncOperationSPtr ManagementSubsystem::BeginCreateEventStoreService(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->BeginCreateEventStoreService(
            timeout,
            callback,
            asyncOperation);
    }

    ErrorCode ManagementSubsystem::EndCreateEventStoreService(AsyncOperationSPtr const & asyncOperation)
    {
        return impl_->EndCreateEventStoreService(asyncOperation);
    }

    ServiceDescription ManagementSubsystem::CreateEventStoreServiceDescription()
    {
        wstring serviceName = *SystemServiceApplicationNameHelper::InternalEventStoreServiceName;
        EventStoreServiceConfig serviceConfig;

        StringMap configSection;
        ManagementConfig::GetConfig().GetKeyValues(serviceName, configSection);

        if (!serviceConfig.TryReadFromConfig(serviceName, configSection))
        {
                WriteInfo(
                    TraceManagementSubsystem,
                    "Configuration section '{0}' is not found or has invalid parameters; ignoring this instance",
                    serviceName);
        }

        return ServiceDescription(
            SystemServiceApplicationNameHelper::InternalEventStoreServiceName,
            0, // instance
            0, // update version
            1, // partition count
            serviceConfig.TargetReplicaSetSize, // Target replica size
            serviceConfig.MinReplicaSetSize, // Min replica size
            true, // Is stateful
            true, // Is persisted
            ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration,
            ServiceModelConfig::GetConfig().QuorumLossWaitDuration,
            ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration,
            *ServiceTypeIdentifier::EventStoreServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            serviceConfig.PlacementConstraints, // placement constraints
            0, // scaleout count
            vector<Reliability::ServiceLoadMetricDescription>(),
            FABRIC_MOVE_COST_LOW,
            vector<byte>(),
            SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    ErrorCode ManagementSubsystem::OnOpen()
    {
        return impl_->Open();
    }

    ErrorCode ManagementSubsystem::OnClose()
    {
        return impl_->Close();
    }

    void ManagementSubsystem::OnAbort()
    {
        impl_->Abort();
    }

    ErrorCode ManagementSubsystem::SetClusterSecurity(SecuritySettings const & value)
    {
        return impl_->SetClusterSecurity(value);
    }

    ErrorCode ManagementSubsystem::SetKeepAlive(ULONG keepAliveIntervalInSeconds)
    {
        return impl_->SetKeepAlive(keepAliveIntervalInSeconds);
    }
}