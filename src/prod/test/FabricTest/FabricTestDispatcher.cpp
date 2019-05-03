// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancingPublic.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancing.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancingTestHelper.h"
#include "client/ClientConfig.h"
#include "Transport/TransportConfig.h"
#include "Transport/Throttle.h"

using namespace Api;
using namespace FabricTest;
using namespace Client;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ReliabilityTestApi;
using namespace ReliabilityTestApi::FailoverManagerComponentTestApi;
using namespace std;
using namespace Federation;
using namespace Common;
using namespace ServiceModel;
using namespace Store;
using namespace Transport;
using namespace Naming;
using namespace Management;
using namespace Management::HealthManager;
using namespace FederationTestCommon;
using namespace TestCommon;
using namespace TestHooks;
using namespace Hosting2;
using namespace HttpGateway;
using namespace Query;

wstring FabricTestDispatcher::TestDataDirectory = L"FabricTestData";

vector<Reliability::ServiceCorrelationDescription> const FabricTestDispatcher::DefaultCorrelations;
wstring const FabricTestDispatcher::DefaultPlacementConstraints = L"";
int const FabricTestDispatcher::DefaultScaleoutCount = 0;
vector<Reliability::ServiceLoadMetricDescription> const FabricTestDispatcher::DefaultServiceMetrics;
uint const FabricTestDispatcher::DefaultMoveCost = 0;

bool FabricTestDispatcher::UseBackwardsCompatibleClients = false;

int const FabricTestDispatcher::ClientActivityCheckIntervalInMilliseconds = 15000;

wstring const FabricTestDispatcher::ParamDelimiter = L" ";
wstring const FabricTestDispatcher::ItemDelimiter = L",";
wstring const FabricTestDispatcher::KeyValueDelimiter = L"=";
wstring const FabricTestDispatcher::ServiceFUDelimiter = L"#";
wstring const FabricTestDispatcher::StatePropertyDelimiter = L".";
wstring const FabricTestDispatcher::GuidDelimiter = L"-";

wstring const FabricTestDispatcher::ServiceTypeStateful = L"Stateful";
wstring const FabricTestDispatcher::ServiceTypeStateless = L"Stateless";

wstring const FabricTestDispatcher::PartitionKindSingleton = L"FABRIC_SERVICE_PARTITION_KIND_SINGLETON";
wstring const FabricTestDispatcher::PartitionKindInt64Range = L"FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE";
wstring const FabricTestDispatcher::PartitionKindName = L"FABRIC_SERVICE_PARTITION_KIND_NAMED";

wstring const FabricTestDispatcher::ReplicaRolePrimary = L"Primary";
wstring const FabricTestDispatcher::ReplicaRoleSecondary = L"Secondary";
wstring const FabricTestDispatcher::ReplicaRoleIdle = L"Idle";
wstring const FabricTestDispatcher::ReplicaRoleNone = L"None";

wstring const FabricTestDispatcher::ReplicaStateStandBy = L"StandBy";
wstring const FabricTestDispatcher::ReplicaStateInBuild = L"InBuild";
wstring const FabricTestDispatcher::ReplicaStateReady = L"Ready";
wstring const FabricTestDispatcher::ReplicaStateDropped = L"Dropped";

wstring const FabricTestDispatcher::ReplicaStatusDown = L"Down";
wstring const FabricTestDispatcher::ReplicaStatusUp = L"Ready";
wstring const FabricTestDispatcher::ReplicaStatusInvalid = L"Invalid";

wstring const FabricTestDispatcher::ServiceReplicaStatusInvalid = L"Invalid";
wstring const FabricTestDispatcher::ServiceReplicaStatusInBuild = L"InBuild";
wstring const FabricTestDispatcher::ServiceReplicaStatusStandBy = L"StandBy";
wstring const FabricTestDispatcher::ServiceReplicaStatusReady = L"Ready";
wstring const FabricTestDispatcher::ServiceReplicaStatusDown = L"Down";
wstring const FabricTestDispatcher::ServiceReplicaStatusDropped = L"Dropped";

StringLiteral const TraceSource("Output");

wstring const GlobalBehavior = L"*";
wstring const MaxTimeValue = L"Max";

wstring const StringDefault = L"default";
int const FabricTestDispatcher::RetryWaitMilliSeconds = 2000;
wstring const NamingServiceName = L"NamingService";

extern HRESULT TryConvertComResult(
    ComPointer<IFabricResolvedServicePartitionResult> const & comResult,
    __out ResolvedServicePartition & cResult);

bool IsTrue(wstring const & input)
{
    return StringUtility::AreEqualCaseInsensitive(input, L"true") ||
        StringUtility::AreEqualCaseInsensitive(input, L"yes") ||
        StringUtility::AreEqualCaseInsensitive(input, L"t") ||
        StringUtility::AreEqualCaseInsensitive(input, L"y");
}

bool IsFalse(wstring const & input)
{
    return StringUtility::AreEqualCaseInsensitive(input, L"false") ||
        StringUtility::AreEqualCaseInsensitive(input, L"no") ||
        StringUtility::AreEqualCaseInsensitive(input, L"f") ||
        StringUtility::AreEqualCaseInsensitive(input, L"n");
}

FabricTestDispatcher::FabricTestDispatcher()
    : isNativeImageStoreEnabled_(false)
    , isTStoreSystemServicesEnabled_(false)
    , isSkipForTStoreEnabled_(false)
    , clientCredentialsType_(L"None")
    , testFederation_()
    , clientsTracker_(testFederation_)
    , commandsTracker_()
{
}

void FabricTestDispatcher::SetSystemServiceStorageProvider()
{
    if (isTStoreSystemServicesEnabled_)
    {
        FabricTestCommonConfig::GetConfig().EnableTStore = isTStoreSystemServicesEnabled_;
        Store::StoreConfig::GetConfig().EnableTStore = isTStoreSystemServicesEnabled_;
        Store::StoreConfig::GetConfig().Test_EnableTStore = isTStoreSystemServicesEnabled_;
    }

    if (Store::StoreConfig::GetConfig().EnableTStore)
    {
        if (FabricTestCommonConfig::GetConfig().IsBackupTest)
        {
            TestSession::WriteWarning(TraceSource, "Backup/Restore not supported on KVS/TStore stack yet - exiting test");

            ExitProcess(0);
        }
        else if (FabricTestCommonConfig::GetConfig().IsFalseProgressTest)
        {
            TestSession::WriteWarning(TraceSource, "False progress tests that leverage blocking replication do not work on KVS/TStore stack - exiting test");

            ExitProcess(0);
        }
    }
}

void FabricTestDispatcher::SetSystemServiceDescriptions()
{
    
    NamingConfig & namingConfig = NamingConfig::GetConfig();
    Management::ManagementConfig & managementConfig = Management::ManagementConfig::GetConfig();

    {
        FailoverConfig & config = FailoverConfig::GetConfig();

        ServiceDescription fmServiceDescription = ServiceDescription(
            ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName,
            0,      /* Instance */
            0,      /* UpdateVersion */
            1,      /* PartitionCount */
            config.TargetReplicaSetSize,
            config.MinReplicaSetSize,
            true,   /* IsStateful */
            true,   /* HasPersistedState */
            config.ReplicaRestartWaitDuration,
            config.FullRebuildWaitDuration,
            config.StandByReplicaKeepDuration,
            *ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId,
            vector<Reliability::ServiceCorrelationDescription>(),
            config.PlacementConstraints,
            0,      /* ScaleoutCount */
            vector<Reliability::ServiceLoadMetricDescription>(),
            1,      /* DefaultMoveCost */
            vector<byte>());

        PartitionedServiceDescriptor psd;
        TestSession::FailTestIfNot(PartitionedServiceDescriptor::Create(fmServiceDescription, psd).IsSuccess(), "Failed to create FM PSD");
        ServiceMap.AddPartitionedServiceDescriptor(ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName, move(psd), true);
    }

    vector<Reliability::ServiceLoadMetricDescription> serviceMetricDescriptions;
    serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
        Naming::Constants::NamingServicePrimaryCountName,
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
        1,
        0));
    serviceMetricDescriptions.push_back(Reliability::ServiceLoadMetricDescription(
        Naming::Constants::NamingServiceReplicaCountName,
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
        1,
        1));

    namingServiceDescription_ = ServiceDescription(
        ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName,
        DateTime::Now().Ticks,
        0, /* ServiceUpdateVersion */
        namingConfig.PartitionCount,
        namingConfig.TargetReplicaSetSize,
        namingConfig.MinReplicaSetSize,
        true,
        true,
        namingConfig.ReplicaRestartWaitDuration,
        namingConfig.QuorumLossWaitDuration,
        namingConfig.StandByReplicaKeepDuration,
        *ServiceModel::ServiceTypeIdentifier::NamingStoreServiceTypeId,
        vector<Reliability::ServiceCorrelationDescription>(),
        namingConfig.PlacementConstraints,
        Naming::Constants::ScaleoutCount,
        move(serviceMetricDescriptions),
        Naming::Constants::DefaultMoveCost,
        vector<byte>());

    {
        PartitionedServiceDescriptor psd;
        TestSession::FailTestIfNot(PartitionedServiceDescriptor::Create(namingServiceDescription_, 0, namingServiceDescription_.PartitionCount - 1, psd).IsSuccess(), "Failed to create Naming PSD");
        ServiceMap.AddPartitionedServiceDescriptor(ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName, move(psd), true);
    }

    if(managementConfig.TargetReplicaSetSize > 0)
    {
        clusterManagerServiceDescription_ = Management::ManagementSubsystem::CreateClusterManagerServiceDescription(
            managementConfig.TargetReplicaSetSize,
            managementConfig.MinReplicaSetSize,
            managementConfig.ReplicaRestartWaitDuration,
            managementConfig.QuorumLossWaitDuration,
            managementConfig.StandByReplicaKeepDuration,
            managementConfig.PlacementConstraints);

        PartitionedServiceDescriptor psd;
        TestSession::FailTestIfNot(PartitionedServiceDescriptor::Create(clusterManagerServiceDescription_, psd).IsSuccess(), "Failed to create CM PSD");
        ServiceMap.AddPartitionedServiceDescriptor(ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName, move(psd), true);
    }

    // RM service descriptor is added in SetRepairManagerServiceProperties, to support upgrade testing
    // (creating the service after the first node has come up)

    if (this->IsNativeImageStoreEnabled)
    {
        fileStoreServiceDescription_ = Management::ManagementSubsystem::CreateImageStoreServiceDescription();

        PartitionedServiceDescriptor psd;
        TestSession::FailTestIfNot(PartitionedServiceDescriptor::Create(fileStoreServiceDescription_, psd).IsSuccess(), "Failed to create Image Store PSD");

        ServiceMap.AddPartitionedServiceDescriptor(fileStoreServiceDescription_.Name, move(psd), true);
    }

    infrastructureServiceDescriptions_.clear();
    Management::ManagementSubsystem::LoadSystemServiceDescriptions(
        infrastructureServiceDescriptions_,
        SystemServiceApplicationNameHelper::InternalInfrastructureServiceName);

    for (auto it = infrastructureServiceDescriptions_.begin(); it != infrastructureServiceDescriptions_.end(); ++it)
    {
        Reliability::ServiceDescription const & serviceDescription = *it;

        if (serviceDescription.TargetReplicaSetSize > 0)
        {
            PartitionedServiceDescriptor psd;
            TestSession::FailTestIfNot(PartitionedServiceDescriptor::Create(serviceDescription, psd).IsSuccess(), "Failed to create IS PSD");
            ServiceMap.AddPartitionedServiceDescriptor(serviceDescription.Name, move(psd), true);
        }
    }
}

void FabricTestDispatcher::SetSharedLogSettings()
{
    if (FabricTestCommonConfig::GetConfig().EnableHashSharedLogId)
    {
        KtlLogger::KtlLoggerConfig::GetConfig().EnableHashSystemSharedLogIdFromPath = true;

        FabricTestCommonConfig::GetConfig().EnableHashSharedLogIdFromPath = true;
        Reliability::FailoverConfig::GetConfig().EnableHashSharedLogIdFromPath = true;
        Naming::NamingConfig::GetConfig().EnableHashSharedLogIdFromPath = true;
        Management::ManagementConfig::GetConfig().EnableHashSharedLogIdFromPath = true;
        Management::FileStoreService::FileStoreServiceConfig::GetConfig().EnableHashSharedLogIdFromPath = true;
        Management::RepairManager::RepairManagerConfig::GetConfig().EnableHashSharedLogIdFromPath = true;
    }

    if (FabricTestCommonConfig::GetConfig().EnableSparseSharedLogs)
    {
        KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogCreateFlags = 1;

        FabricTestCommonConfig::GetConfig().CreateFlags = 1;
        Reliability::FailoverConfig::GetConfig().CreateFlags = 1;
        Naming::NamingConfig::GetConfig().CreateFlags = 1;
        Management::ManagementConfig::GetConfig().CreateFlags = 1;
        Management::FileStoreService::FileStoreServiceConfig::GetConfig().CreateFlags = 1;
        Management::RepairManager::RepairManagerConfig::GetConfig().CreateFlags = 1;
    }

    auto sharedLogSize = FabricTestCommonConfig::GetConfig().SharedLogSize;
    if (sharedLogSize > 0)
    {
        KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogSizeInMB = sharedLogSize / (1024 * 1024);

        FabricTestCommonConfig::GetConfig().LogSize = sharedLogSize;
        Reliability::FailoverConfig::GetConfig().LogSize = sharedLogSize;
        Naming::NamingConfig::GetConfig().LogSize = sharedLogSize;
        Management::ManagementConfig::GetConfig().LogSize = sharedLogSize;
        Management::FileStoreService::FileStoreServiceConfig::GetConfig().LogSize = sharedLogSize;
        Management::RepairManager::RepairManagerConfig::GetConfig().LogSize = sharedLogSize;
    }

    auto maxRecordSize = FabricTestCommonConfig::GetConfig().SharedLogMaximumRecordSize;
    if (maxRecordSize > 0)
    {
        KtlLogger::KtlLoggerConfig::GetConfig().SystemSharedLogMaximumRecordSizeInKB = maxRecordSize / 1024;

        FabricTestCommonConfig::GetConfig().MaximumRecordSize = maxRecordSize;
        Reliability::FailoverConfig::GetConfig().MaximumRecordSize = maxRecordSize;
        Naming::NamingConfig::GetConfig().MaximumRecordSize = maxRecordSize;
        Management::ManagementConfig::GetConfig().MaximumRecordSize = maxRecordSize;
        Management::FileStoreService::FileStoreServiceConfig::GetConfig().MaximumRecordSize = maxRecordSize;
        Management::RepairManager::RepairManagerConfig::GetConfig().MaximumRecordSize = maxRecordSize;
    }
}

bool FabricTestDispatcher::Open()
{
    //Default load balancing domain always exists and has no load metrics
    FailoverConfig::GetConfig().FMStoreDirectory = FabricTestConstants::FMDataDirectory;
    clientsStopped_ = true;
    fabricClient_ = make_shared<TestFabricClient>();
    fabricClientHealth_ = make_unique<TestFabricClientHealth>(*this);
    fabricClientQuery_ = make_unique<TestFabricClientQuery>();
    queryExecutor_ = make_unique<FabricTestQueryExecutor>();
    fabricClientSettings_ = make_unique<TestFabricClientSettings>();
    fabricClientUpgrade_ = make_unique<TestFabricClientUpgrade>(fabricClient_);
    fabricClientScenarios_ = make_unique<TestFabricClientScenarios>(*this, fabricClient_);
    nativeImageStoreExecutor_ = make_unique<NativeImageStoreExecutor>(*this);
    checkpointTruncationTimestampValidator_ = make_unique<CheckpointTruncationTimestampValidator>(*this);

    SetCommandHandlers();

    fabricActivationManager_ = make_shared<FabricActivationManager>(true, true);
    auto error = OpenFabricActivationManager();
    if(!error.IsSuccess())
    {
        return false;
    }

    return true;
}

ErrorCode FabricTestDispatcher::OpenFabricActivationManager()
{
   AsyncOperationWaiterSPtr activateWaiter = make_shared<AsyncOperationWaiter>();

   auto activationManager = fabricActivationManager_;

    fabricActivationManager_->BeginOpen(
        TimeSpan::FromSeconds(60),
        [activateWaiter, activationManager](AsyncOperationSPtr const & operation)
    {
        auto error = activationManager->EndOpen(operation);
        activateWaiter->SetError(error);
        activateWaiter->Set();
    },
    FABRICSESSION.CreateAsyncOperationRoot());
    ErrorCode error(ErrorCodeValue::Success);
    if(activateWaiter->WaitOne(TimeSpan::FromSeconds(60)))
    {
        error = activateWaiter->GetError();
    }
    else
    {
        error = ErrorCode(ErrorCodeValue::InvalidState);
    }
    return error;
}

void FabricTestDispatcher::Close()
{
    TestSession::FailTestIfNot(commandsTracker_.IsEmpty(), "Must 'waitasync' all pending async commands before closing");

    // The close for all nodes on quit was added as a work around while investigating static issues when main process
    // was exiting. There were two product issues found and fixed hence am effectively reverting the change to preemptively close 
    // all nodes before exiting the process. I am leaving this code behind in case it is needed in the future

    if(FabricTestSessionConfig::GetConfig().CloseAllNodesOnTestQuit)
    {
        TestSession::WriteInfo(TraceSource, "Closing all nodes before quitting test");
        FabricTestSessionConfig::GetConfig().AsyncNodeCloseEnabled = false;
        CloseAllNodes(false/*close*/);
    }

    auto error = CloseFabricActivationManager();
    if(!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "FabricActivationManager close failed. Error {0}", error);
    }

    this->CloseHttpGateway();
}

bool FabricTestDispatcher::IsFabricActivationManagerOpen()
{
    return fabricActivationManager_->State.Value == FabricComponentState::Opened;
}

ErrorCode FabricTestDispatcher::CloseFabricActivationManager()
{
    auto activationManager = fabricActivationManager_;
    AsyncOperationWaiterSPtr opWaiter = make_shared<AsyncOperationWaiter>();
     fabricActivationManager_->BeginClose(
        TimeSpan::FromSeconds(120),
        [opWaiter, activationManager](AsyncOperationSPtr const & operation)
    {
        auto error = activationManager->EndClose(operation);
        opWaiter->SetError(error);
        opWaiter->Set();
    },
    FABRICSESSION.CreateAsyncOperationRoot());
    ErrorCode error(ErrorCodeValue::Success);
    if(opWaiter->WaitOne(TimeSpan::FromSeconds(120)))
    {
        error = opWaiter->GetError();
    }
    else
    {
        error = ErrorCode(ErrorCodeValue::InvalidState);
    }
    return error;
}

void FabricTestDispatcher::SetCommandHandlers()
{
    commandHandlers_[FabricTestCommands::AddCommand] = [this](Common::StringCollection const & paramCollection)
    {
        return AddNode(paramCollection);
    };

    commandHandlers_[FabricTestCommands::RemoveCommand] = [this](Common::StringCollection const & paramCollection)
    {
        return RemoveNode(paramCollection);
    };

    commandHandlers_[FabricTestCommands::AbortCommand] = [this](Common::StringCollection const & paramCollection)
    {
        if (paramCollection.size() != 1)
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::AbortCommand);
            return false;
        }

        return AbortNode(paramCollection[0]);
    };
    commandHandlers_[FabricTestCommands::ClearTicketCommand] = [this](StringCollection const & paramCollection)
    {
        return ClearTicket(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ResetStoreCommand] = [this](StringCollection const & paramCollection)
    {
        return ResetStore(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CleanTestCommand] = [this](StringCollection const &)
    {
        return CleanTest();
    };
    commandHandlers_[FabricTestCommands::EnableNativeImageStore] = [this](StringCollection const &)
    {
        isNativeImageStoreEnabled_ = true;
        return true;
    };
    commandHandlers_[FabricTestCommands::VerifyImageStore] = [this](StringCollection const & paramCollection)
    {
        return VerifyImageStore(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyNodeFiles] = [this](StringCollection const & paramCollection)
    {
        return VerifyNodeFiles(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetPropertyCommand] = [this](StringCollection const & paramCollection)
    {
        return SetProperty(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineFailoverManagerServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetFailoverManagerServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineNamingServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetNamingServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineClusterManagerServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetClusterManagerServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineImageStoreServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetImageStoreServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineInfrastructureServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetInfrastructureServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveInfrastructureServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return RemoveInfrastructureServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineRepairManagerServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetRepairManagerServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineDnsServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetDnsServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineNIMServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return SetNIMServiceProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DefineEnableUnsupportedPreviewFeaturesCommand] = [this](StringCollection const & paramCollection)
    {
        return SetEnableUnsupportedPreviewFeatures(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyAll(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ListCommand] = [this](StringCollection const &)
    {
        return ListNodes();
    };
    commandHandlers_[FabricTestCommands::FMCommand] = [this](StringCollection const &)
    {
        return ShowFM();
    };
    commandHandlers_[FabricTestCommands::UpdateServicePackageVersionInstanceCommand] = [this](StringCollection const & paramCollection)
    {
        return UpdateServicePackageVersionInstance(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ShowNodesCommand] = [this](StringCollection const &)
    {
        return ShowNodes();
    };
    commandHandlers_[FabricTestCommands::ShowGFUMCommand] = [this](StringCollection const & paramCollection)
    {
        return ShowGFUM(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ShowLFUMCommand] = [this](StringCollection const & paramCollection)
    {
        return ShowLFUM(paramCollection);
    };
    commandHandlers_[L"service"] = [this](StringCollection const &)
    {
        return ShowService();
    };
    commandHandlers_[L"servicetype"] = [this](StringCollection const &)
    {
        return ShowServiceType();
    };
    commandHandlers_[L"application"] = [this](StringCollection const &)
    {
        return ShowApplication();
    };
    commandHandlers_[L"hm"] = [this](StringCollection const & paramCollection)
    {
        return ShowHM(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateServiceGroupCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CreateServiceGroup(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpdateServiceGroupCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.UpdateServiceGroup(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteServiceGroupCommand] = [this](StringCollection const & paramCollection)
    {
        return DeleteServiceGroup(paramCollection);
    };
    commandHandlers_[FabricTestCommands::InjectFailureCommand] = [this](StringCollection const & paramCollection)
    {
        return InjectFailure(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveFailureCommand] = [this](StringCollection const & paramCollection)
    {
        return RemoveFailure(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetSignalCommand] = [this](StringCollection const & paramCollection)
    {
        return SetSignal(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ResetSignalCommand] = [this](StringCollection const & paramCollection)
    {
        return ResetSignal(paramCollection);
    };
    commandHandlers_[FabricTestCommands::WaitForSignalHitCommand] = [this](StringCollection const & paramCollection)
    {
        return WaitForSignalHit(paramCollection);
    };
    commandHandlers_[FabricTestCommands::FailIfSignalHitCommand] = [this](StringCollection const & paramCollection)
    {
        return FailIfSignalHit(paramCollection);
    };
    commandHandlers_[FabricTestCommands::WaitAsyncCommand] = [this](StringCollection const & paramCollection)
    {
        return WaitAsync(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CreateService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateServiceFromTemplateCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CreateServiceFromTemplate(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpdateServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.UpdateService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return DeleteService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::GetServiceDescriptionCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.GetServiceDescription(paramCollection);
    };
    commandHandlers_[FabricTestCommands::GetServiceDescriptionUsingClientCommand] = [this](StringCollection const & paramCollection)
    {
        return GetServiceDescriptionUsingClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::GetServiceGroupDescriptionCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.GetServiceGroupDescription(paramCollection);
    };
    commandHandlers_[FabricTestCommands::AddRuntimeCommand] = [this](StringCollection const & paramCollection)
    {
        return AddRuntime(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveRuntimeCommand] = [this](StringCollection const & paramCollection)
    {
        return RemoveRuntime(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UnregisterRuntimeCommand] = [this](StringCollection const & paramCollection)
    {
        return UnregisterRuntime(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RegisterServiceTypeCommand] = [this](StringCollection const & paramCollection)
    {
        return RegisterServiceType(paramCollection);
    };
    commandHandlers_[FabricTestCommands::EnableServiceTypeCommand] = [this](StringCollection const & paramCollection)
    {
        return EnableServiceType(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DisableServiceTypeCommand] = [this](StringCollection const & paramCollection)
    {
        return DisableServiceType(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ShowActiveServicesCommand] = [this](StringCollection const & paramCollection)
    {
        return ShowActiveServices(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ShowLoadMetricInfoCommand] = [this](StringCollection const & paramCollection)
    {
        return ShowLoadMetricInfo(paramCollection);
    };
    commandHandlers_[FabricTestCommands::EnumerateNamesCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.EnumerateNames(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateNameCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CreateName(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteNameCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.DeleteName(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DnsNameExistsCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.DnsNameExists(paramCollection);
    };
    commandHandlers_[FabricTestCommands::NameExistsCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.NameExists(paramCollection);
    };
    commandHandlers_[FabricTestCommands::PutPropertyCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.PutProperty(paramCollection);
    };
    commandHandlers_[FabricTestCommands::PutCustomPropertyCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.PutCustomProperty(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SubmitPropertyBatchCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.SubmitPropertyBatch(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeletePropertyCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.DeleteProperty(paramCollection);
    };
    commandHandlers_[FabricTestCommands::GetPropertyCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.GetProperty(paramCollection);
    };
    commandHandlers_[FabricTestCommands::GetMetadataCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.GetMetadata(paramCollection);
    };
    commandHandlers_[FabricTestCommands::EnumeratePropertiesCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.EnumerateProperties(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyPropertyEnumerationCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.VerifyPropertyEnumeration(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ResolveServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return ResolveService(paramCollection, nullptr);
    };
    commandHandlers_[FabricTestCommands::ResolveUsingClientCommand] = [this](StringCollection const & paramCollection)
    {
        return ResolveServiceUsingClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ResolveServiceWithEventsCommand] = [this](StringCollection const & paramCollection)
    {
        return ResolveServiceWithEvents(paramCollection);
    };
    commandHandlers_[FabricTestCommands::PrefixResolveServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return PrefixResolveService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateServiceNotificationClient] = [this](StringCollection const & paramCollection)
    {
        return CreateServiceNotificationClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RegisterServiceNotificationFilter] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.RegisterServiceNotificationFilter(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UnregisterServiceNotificationFilter] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.UnregisterServiceNotificationFilter(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetServiceNotificationWait] = [this](StringCollection const & paramCollection)
    {
        return SetServiceNotificationWait(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ServiceNotificationWait] = [this](StringCollection const & paramCollection)
    {
        return ServiceNotificationWait(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientPutCommand] = [this](StringCollection const & paramCollection)
    {
        return ClientPut(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientGetCommand] = [this](StringCollection const & paramCollection)
    {
        return ClientGet(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientGetAllCommand] = [this](StringCollection const & paramCollection)
    {
        return ClientGetAll(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientGetKeysCommand] = [this](StringCollection const & paramCollection)
    {
        return ClientGetKeys(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientDeleteCommand] = [this](StringCollection const & paramCollection)
    {
        return ClientDelete(paramCollection);
    };
    commandHandlers_[FabricTestCommands::StartClientCommand] = [this](StringCollection const & paramCollection)
    {
        return StartClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::StopClientCommand] = [this](StringCollection const & paramCollection)
    {
        return StopClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientBackup] = [this](StringCollection const & paramCollection)
    {
        return ClientBackup(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientRestore] = [this](StringCollection const & paramCollection)
    {
        return ClientRestore(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientCompressionTest] = [this](StringCollection const & paramCollection)
    {
        return ClientCompression(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UseBackwardsCompatibleClientsCommand] = [this](StringCollection const & paramCollection)
    {
        return SetBackwardsCompatibleClients(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeactivateNodeCommand] = [this](StringCollection const & paramCollection)
    {
        NodeId nodeToUpdate; bool updateNode = false;
        bool rv = FabricClient.DeactivateNode(paramCollection, updateNode, nodeToUpdate);

        if (rv && updateNode)
        {
            auto node = testFederation_.GetNode(nodeToUpdate);
            if (node)
            {
                node->IsActivated = false;
            }
        }

        return rv;
    };
    commandHandlers_[FabricTestCommands::DeactivateNodesCommand] = [this](StringCollection const & paramCollection)
    {
        return DeactivateNodes(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveNodeDeactivationCommand] = [this](StringCollection const& paramCollection)
    {
        return RemoveNodeDeactivation(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyNodeDeactivationStatusCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyNodeDeactivationStatus(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ActivateNodeCommand] = [this](StringCollection const & paramCollection)
    {
        NodeId nodeToUpdate;
        bool rv = FabricClient.ActivateNode(paramCollection, nodeToUpdate);

        if (rv)
        {
            auto node = testFederation_.GetNode(nodeToUpdate);
            if (node)
            {
                node->IsActivated = true;
            }
        }

        return rv;
    };
    commandHandlers_[FabricTestCommands::NodeStateRemovedCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.NodeStateRemoved(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpdateNodeImagesCommand] = [this](StringCollection const & paramCollection)
    {
        return UpdateNodeImages(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RecoverPartitionsCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.RecoverPartitions(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RecoverPartitionCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.RecoverPartition(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RecoverServicePartitionsCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.RecoverServicePartitions(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ClientReportFaultCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.ReportFault(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RecoverSystemPartitionsCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.RecoverSystemPartitions(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ResetPartitionLoadCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.ResetPartitionLoad(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ToggleVerboseServicePlacementHealthReportingCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.ToggleVerboseServicePlacementHealthReporting(paramCollection);
    };
    commandHandlers_[FabricTestCommands::StartTestFabricClientCommand] = [this](StringCollection const & paramCollection)
    {
        return StartTestFabricClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::StopTestFabricClientCommand] = [this](StringCollection const &)
    {
        return StopTestFabricClient();
    };
    commandHandlers_[FabricTestCommands::ProcessPendingPlbUpdates] = [this](StringCollection const & paramCollection)
    {
        return ProcessPendingPlbUpdates(paramCollection);
    };
    commandHandlers_[FabricTestCommands::PLBUpdateService] = [this](StringCollection const & paramCollection)
    {
        return PLBUpdateService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SwapPrimaryCommand] = [this](StringCollection const & paramCollection)
    {
        return SwapPrimary(paramCollection);
    };
    commandHandlers_[FabricTestCommands::PromoteToPrimaryCommand] = [this](StringCollection const & paramCollection)
    {
        return PromoteToPrimary(paramCollection);
    };
    commandHandlers_[FabricTestCommands::MovePrimaryFromClientCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.MovePrimaryReplicaFromClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::MoveSecondaryFromClientCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.MoveSecondaryReplicaFromClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::MoveSecondaryCommand] = [this](StringCollection const & paramCollection)
    {
        return MoveSecondary(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateClientCommand] = [this](StringCollection const & paramCollection)
    {
        return CreateNamedFabricClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteClientCommand] = [this](StringCollection const & paramCollection)
    {
        return DeleteNamedFabricClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RegisterCallbackCommand] = [this](StringCollection const & paramCollection)
    {
        return RegisterCallback(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UnregisterCallbackCommand] = [this](StringCollection const & paramCollection)
    {
        return UnregisterCallback(paramCollection);
    };
    commandHandlers_[FabricTestCommands::WaitForCallbackCommand] = [this](StringCollection const & paramCollection)
    {
        return WaitForCallback(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyCachedServiceResolutionCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyCacheItem(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyFabricTimeCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyFabricTime(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveCachedServiceResolutionCommand] = [this](StringCollection const & paramCollection)
    {
        return RemoveClientCacheItem(paramCollection);
    };
    commandHandlers_[FabricTestCommands::PerformanceTestCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.PerformanceTest(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CallService] = [this](StringCollection const & paramCollection)
    {
        return CallService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ScenariosCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientScenarios.RunScenario(paramCollection);
    };
    //------------------UpgradeApplicationType----------------------------------------------------
    commandHandlers_[FabricTestCommands::ParallelProvisionApplicationTypeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.ParallelProvisionApplicationType(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ProvisionApplicationTypeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.ProvisionApplicationType(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UnprovisionApplicationTypeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UnprovisionApplicationType(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateApplicationCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.CreateApplication(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpdateApplicationCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpdateApplication(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteApplicationCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.DeleteApplication(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeApplicationCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpgradeApplication(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpdateApplicationUpgradeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpdateApplicationUpgrade(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RollbackApplicationUpgradeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.RollbackApplicationUpgrade(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyApplicationUpgradeDescriptionCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.VerifyApplicationUpgradeDescription(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetRollbackApplicationCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.SetRollbackApplication(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeAppStatusCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpgradeAppStatus(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeAppMoveNext] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.MoveNextApplicationUpgradeDomain(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateComposeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.CreateComposeDeployment(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteComposeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.DeleteComposeDeployment(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeComposeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpgradeComposeDeployment(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RollbackComposeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.RollbackComposeDeployment(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CreateNetworkCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CreateNetwork(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteNetworkCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.DeleteNetwork(paramCollection);
    };
    commandHandlers_[FabricTestCommands::GetNetworkCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.GetNetwork(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ShowNetworksCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.ShowNetworks(paramCollection);
    };
    commandHandlers_[FabricTestCommands::KillCodePackageCommand] = [this](StringCollection const & paramCollection)
    {
        return KillCodePackage(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetCodePackageKillPendingCommand] = [this](StringCollection const & paramCollection)
    {
        return SetCodePackageKillPending(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyUpgradeAppCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyUpgradeApp(paramCollection);
    };
    //------------------end of UpgradeApplicationType----start of upgradeFabric---------------------------------

    commandHandlers_[FabricTestCommands::PrepareUpgradeFabricCommand] = [this](StringCollection const & paramCollection)
    {
        return PrepareUpgradeFabric(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ProvisionFabricCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.ProvisionFabric(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UnprovisionFabricCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UnprovisionFabric(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeFabricCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpgradeFabric(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpdateFabricUpgradeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpdateFabricUpgrade(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RollbackFabricUpgradeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.RollbackFabricUpgrade(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyFabricUpgradeDescriptionCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.VerifyFabricUpgradeDescription(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetRollbackFabricCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.SetRollbackFabric(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeFabricStatusCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.UpgradeFabricStatus(paramCollection);
    };
    commandHandlers_[FabricTestCommands::UpgradeFabricMoveNext] = [this](StringCollection const & paramCollection)
    {
        return FabricClientUpgrade.MoveNextFabricUpgradeDomain(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyUpgradeFabricCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyUpgradeFabric(paramCollection);
    };
    commandHandlers_[FabricTestCommands::FailFabricUpgradeDownloadCommand] = [this](StringCollection const & paramCollection)
    {
        return FailFabricUpgradeDownload(paramCollection);
    };
    commandHandlers_[FabricTestCommands::FailFabricUpgradeValidationCommand] = [this](StringCollection const & paramCollection)
    {
        return FailFabricUpgradeValidation(paramCollection);
    };
    commandHandlers_[FabricTestCommands::FailFabricUpgradeCommand] = [this](StringCollection const & paramCollection)
    {
        return FailFabricUpgrade(paramCollection);
    };
    //------------------end of upgradefabric--------------------------------------------------------------------
    commandHandlers_[FabricTestCommands::QueryCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientQuery.Query(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ExecuteQueryCommand] = [this](StringCollection const & paramCollection)
    {
        return QueryExecutor.ExecuteQuery(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ReportFaultCommand] = [this](StringCollection const & paramCollection)
    {
        return ReportFault(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ReportLoadCommand] = [this](StringCollection const & paramCollection)
    {
        return ReportLoad(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyLoadReportCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyLoadReport(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyLoadValueCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyLoadValue(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyNodeLoadCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyNodeLoad(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyPartitionLoadCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyPartitionLoad(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyClusterLoadCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyClusterLoad(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyUnplacedReasonCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyUnplacedReason(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyApplicationLoadCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyApplicationLoad(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyResourceOnNodeCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyResourceOnNode(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyLimitsEnforcedCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyLimitsEnforced(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyPLBAndLRMSync] = [this](StringCollection const & paramCollection)
    {
        return VerifyPLBAndLRMSync(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyContainerPodsCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyContainerPods(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyMoveCostValueCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyMoveCostValue(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ReportMoveCostCommand] = [this](StringCollection const & paramCollection)
    {
        return ReportMoveCost(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyReadWriteStatusCommand] = [this](StringCollection const & paramCollection)
    {
        return VerifyReadWriteStatus(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckIfLFUPMIsEmptyCommand] = [this](StringCollection const & paramCollection)
    {
        return CheckIfLfupmIsEmpty(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ReportHealthCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.ReportHealth(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ReportHealthIpcCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.ReportHealthIpc(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ReportHealthInternalCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.ReportHealthInternal(paramCollection);
    };
    commandHandlers_[FabricTestCommands::DeleteHealthCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.DeleteHealth(paramCollection);
    };
    commandHandlers_[FabricTestCommands::QueryHealthCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.QueryHealth(paramCollection);
    };
    commandHandlers_[FabricTestCommands::QueryHealthListCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.QueryHealthList(paramCollection);
    };
    commandHandlers_[FabricTestCommands::QueryHealthStateChunkCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.QueryHealthStateChunk(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckHMEntityCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.CheckHMEntity(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CorruptHMEntityCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.CorruptHMEntity(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckHMCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.CheckHM(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetHMThrottleCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.SetHMThrottle(paramCollection);
    };
    commandHandlers_[FabricTestCommands::TakeClusterHealthSnapshotCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.TakeClusterHealthSnapshot(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckIsClusterHealthyCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.CheckIsClusterHealthy(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckHealthClientCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.CheckHealthClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::HealthPreInitializeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.HealthPreInitialize(paramCollection);
    };
    commandHandlers_[FabricTestCommands::HealthPostInitializeCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.HealthPostInitialize(paramCollection);
    };
    commandHandlers_[FabricTestCommands::HealthGetProgressCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.HealthGetProgress(paramCollection);
    };
    commandHandlers_[FabricTestCommands::HealthSkipSequenceCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.HealthSkipSequence(paramCollection);
    };
    commandHandlers_[FabricTestCommands::ResetHealthClientCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.ResetHealthClient(paramCollection);
    };
    commandHandlers_[FabricTestCommands::WatchDogCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.RunWatchDog(paramCollection);
    };
    commandHandlers_[FabricTestCommands::HMLoadTestCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientHealth.HMLoadTest(paramCollection);
    };
    commandHandlers_[FabricTestCommands::SetFabricClientSettingsCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClientSettings.SetSettings(paramCollection);
    };
    commandHandlers_[FabricTestCommands::InfrastructureCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.InfrastructureCommand(paramCollection);
    };

    // ----------------------------- Repair commands -----------------------------
    FabricTestDispatcherHelper::AddRepairCommands(FabricClient, commandHandlers_);
    // ----------------------------- End Repair commands -------------------------

    commandHandlers_[FabricTestCommands::SetSecondaryPumpEnabledCommand] = [this](StringCollection const & paramCollection)
    {
        return SetSecondaryPumpEnabled(paramCollection);
    };
    commandHandlers_[FabricTestCommands::KillServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return KillService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::KillFailoverManagerServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return KillFailoverManagerService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::KillNamingServiceCommand] = [this](StringCollection const & paramCollection)
    {
        return KillNamingService(paramCollection);
    };
    commandHandlers_[FabricTestCommands::EnableTransportThrottleCommand] = [this](StringCollection const & )
    {
        Transport::Throttle::Enable();
        return true;
    };
    commandHandlers_[FabricTestCommands::ReloadTransportThrottleCommand] = [this](StringCollection const &)
    {
        Transport::Throttle::Test_Reload();
        return true;
    };
    commandHandlers_[FabricTestCommands::DebugBreakCommand] = [this](StringCollection const &)
    {
        if (::IsDebuggerPresent())
        {
            TestSession::WriteInfo(TraceSource, "Breaking into debugger");
            ::DebugBreak();
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Debugger not found - skipping dbgbreak");
        }

        return true;
    };
    commandHandlers_[FabricTestCommands::InjectErrorCommand] = [this](StringCollection const & paramCollection)
    {
        return InjectError(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveErrorCommand] = [this](StringCollection const & paramCollection)
    {
        return RemoveError(paramCollection);
    };
    commandHandlers_[FabricTestCommands::HttpGateway] = [this](StringCollection const & paramCollection)
    {
        return OpenHttpGateway(paramCollection);
    };
    commandHandlers_[FabricTestCommands::EseDump] = [this](StringCollection const & paramCollection)
    {
        return EseDump(paramCollection);
    };
    commandHandlers_[FabricTestCommands::XcopyCommand] = [this](StringCollection const & paramCollection)
    {
        return Xcopy(paramCollection);
    };

    //predeployment command
    commandHandlers_[FabricTestCommands::DeployServicePackageCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.DeployServicePackages(paramCollection);
    };

    commandHandlers_[FabricTestCommands::VerifyDeployedCodePackageCountCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.VerifyDeployedCodePackageCount(paramCollection);
    };

    // unreliable transport command
    commandHandlers_[FabricTestCommands::AddBehaviorCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.AddBehavior(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveBehaviorCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.RemoveBehavior(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckUnreliableTransportIsDisabledCommand] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CheckUnreliableTransportIsDisabled(paramCollection);
    };
    commandHandlers_[FabricTestCommands::CheckTransportBehaviorlist] = [this](StringCollection const & paramCollection)
    {
        return FabricClient.CheckTransportBehaviorlist(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VectoredExceptionHandlerRegistration] = [this](StringCollection const &)
    {
#if defined(PLATFORM_UNIX)
        TestSession::WriteInfo(TraceSource, "Not supported");
#else
        FabricTestVectorExceptionHandler::Register();
#endif

        return true;
    };
    commandHandlers_[FabricTestCommands::NewTestCertificates] = [this] (StringCollection const &)
    {
        return NewTestCerts();
    };
    commandHandlers_[FabricTestCommands::UseAdminClient] = [this] (StringCollection const &)
    {
        return UseAdminClientCredential();
    };
    commandHandlers_[FabricTestCommands::UseReadonlyClient] = [this] (StringCollection const &)
    {
        return UseReadonlyClientCredential();
    };
    commandHandlers_[FabricTestCommands::AddLeaseBehaviorCommand] = [this](StringCollection const & paramCollection)
    {
        return AddLeaseBehavior(paramCollection);
    };
    commandHandlers_[FabricTestCommands::RemoveLeaseBehaviorCommand] = [this](StringCollection const & paramCollection)
    {
        return RemoveLeaseBehavior(paramCollection);
    };

    //support for Disk Drives config
    commandHandlers_[FabricTestCommands::AddDiskDriveFoldersCommand] = [this](StringCollection const & paramCollection)
    {
        return AddDiskDriveFolders(paramCollection);
    };
    commandHandlers_[FabricTestCommands::VerifyDeletedDiskDriveFoldersCommand] = [this](StringCollection const &)
    {
        return VerifyDeletedDiskDriveFolders();
    };

    commandHandlers_[FabricTestCommands::WaitForAllToApplyLsn] = [this](StringCollection const & paramCollection)
    {
        return WaitForAllToApplyLsn(paramCollection);
    };

    commandHandlers_[FabricTestCommands::RequestCheckpoint] = [this](StringCollection const & paramCollection)
    {
        return RequestCheckpoint(paramCollection);
    };

    commandHandlers_[FabricTestCommands::CheckContainersAvailable] = [this](StringCollection const & paramCollection)
    {
        return CheckContainers(paramCollection);
    };

    commandHandlers_[FabricTestCommands::SetEseOnly] = [this](StringCollection const &)
    {
        return SetEseOnly();
    };

    commandHandlers_[FabricTestCommands::EnableLogTruncationTimestampValidation] = [this](StringCollection const & paramCollection)
    {
        return EnableLogTruncationTimestampValidation(paramCollection);
    };
}

bool FabricTestDispatcher::ShouldSkipCommand(wstring const & command)
{
    if (command == FabricTestCommands::ClearEseOnly)
    {
        return ClearEseOnly();
    }
    else if (isSkipForTStoreEnabled_ && FabricTestCommonConfig::GetConfig().EnableTStore)
    {
        TestSession::WriteInfo(TraceSource, "TStore enabled for TestPersistedStoreService - skipping command '{0}'", command);

        return true;
    }

    return false;
}

bool FabricTestDispatcher::ExecuteCommand(wstring command)
{
    if (command.empty())
    {
        return false;
    }

    if (CommonDispatcher::ExecuteCommand(command))
    {
        return true;
    }

    StringCollection paramCollection;
    StringUtility::Split<wstring>(command, paramCollection, FabricTestDispatcher::ParamDelimiter);

    if (Common::StringUtility::StartsWith(command, FabricTestCommands::AddCommand) ||
        Common::StringUtility::StartsWith(command, FabricTestCommands::RemoveCommand))
    {
        // extract "+" or "-"
        command = paramCollection.front().front();
        paramCollection.front().erase(0, 1);
    }
    else
    {
        command = paramCollection.front();
        paramCollection.erase(paramCollection.begin());
        paramCollection = CompactParameters(paramCollection);
    }

    StringUtility::ToLower(command);

    if (ApplicationBuilder::IsApplicationBuilderCommand(command))
    {
        return ExecuteApplicationBuilderCommand(command, paramCollection);
    }

    if (NativeImageStoreExecutor::IsNativeImageStoreClientCommand(command))
    {
        return nativeImageStoreExecutor_->ExecuteCommand(command, paramCollection);
    }

    auto commandIter = commandHandlers_.find(command);
    if(commandIter != commandHandlers_.end())
    {
        return commandIter->second(paramCollection);
    }
    else
    {
        return false;
    }
}

bool FabricTestDispatcher::ExecuteApplicationBuilderCommand(
    wstring const & command,
    StringCollection const & params)
{
    ComPointer<IFabricNativeImageStoreClient> clientCPtr;

    if (this->IsNativeImageStoreEnabled && command == ApplicationBuilder::ApplicationBuilderUpload)
    {
        this->CreateNativeImageStoreClient(clientCPtr);
    }

    return ApplicationBuilder::ExecuteCommand(command, params, clientCredentialsType_, clientCPtr);
}

void FabricTestDispatcher::PrintHelp(std::wstring const & command)
{
    FABRICSESSION.PrintHelp(command);
}

bool FabricTestDispatcher::DeleteService(Common::StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteServiceCommand);
        return false;
    }

    wstring serviceName = params[0];

    CommandLineParser parser(params, 1);

    vector<wstring> errorStrings;
    parser.GetVector(L"errors", errorStrings, L"Success");
    vector<HRESULT> expectedErrors;
    for (auto iter = errorStrings.begin(); iter != errorStrings.end(); ++iter)
    {
        ErrorCodeValue::Enum error = ParseErrorCode(*iter);
        expectedErrors.push_back(ErrorCode(error).ToHResult());
    }

    bool isForce = parser.GetBool(L"isForce");

    bool success;
    wstring nodeIdString;
    ComPointer<IFabricServiceManagementClient5> serviceClient;
    if (parser.TryGetString(L"nodeconfig", nodeIdString, L""))
    {
        CreateFabricClientOnNode(nodeIdString, /*out*/serviceClient);
    }

    success = FabricClient.DeleteService(serviceName, isForce, expectedErrors, serviceClient);

    if (!success)
    {
        return false;
    }

    bool expectSuccess = find(expectedErrors.begin(), expectedErrors.end(), S_OK) != expectedErrors.end();
    bool expectNotFound = find(expectedErrors.begin(), expectedErrors.end(), FABRIC_E_SERVICE_DOES_NOT_EXIST) != expectedErrors.end();

    if (expectSuccess || expectNotFound)
    {
        FABRICSESSION.FabricDispatcher.ServiceMap.MarkServiceAsDeleted(
            SystemServiceApplicationNameHelper::IsPublicServiceName(serviceName)? SystemServiceApplicationNameHelper::GetInternalServiceName(serviceName): serviceName
            , !expectNotFound);
    }
    return true;
}

bool FabricTestDispatcher::DeleteServiceGroup(Common::StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteServiceGroupCommand);
        return false;
    }

    wstring serviceName = params[0];

    CommandLineParser parser(params, 1);

    vector<wstring> errorStrings;
    parser.GetVector(L"errors", errorStrings, L"Success");
    vector<HRESULT> expectedErrors;
    for (auto iter = errorStrings.begin(); iter != errorStrings.end(); ++iter)
    {
        ErrorCodeValue::Enum error = ParseErrorCode(*iter);
        expectedErrors.push_back(ErrorCode(error).ToHResult());
    }

    bool success;

    success = FabricClient.DeleteServiceGroup(serviceName, expectedErrors);

    if (!success)
    {
        return false;
    }

    bool expectSuccess = find(expectedErrors.begin(), expectedErrors.end(), S_OK) != expectedErrors.end();
    bool expectNotFound = find(expectedErrors.begin(), expectedErrors.end(), FABRIC_E_SERVICE_GROUP_DOES_NOT_EXIST) != expectedErrors.end();

    if (expectSuccess || expectNotFound)
    {
        FABRICSESSION.FabricDispatcher.ServiceMap.MarkServiceAsDeleted(serviceName, !expectNotFound);
    }
    return true;
}

 NodeDeactivationIntent::Enum FabricTestDispatcher::ParseNodeDeactivationIntent(std::wstring const& intentString)
 {
     NodeDeactivationIntent::Enum intent = NodeDeactivationIntent::None;

     if (intentString == L"None")
     {
         intent = NodeDeactivationIntent::None;
     }
     else if (intentString == L"Pause")
     {
         intent = NodeDeactivationIntent::Pause;
     }
     else if (intentString == L"Restart")
     {
         intent = NodeDeactivationIntent::Restart;
     }
     else if (intentString == L"RemoveData")
     {
         intent = NodeDeactivationIntent::RemoveData;
     }
     else if (intentString == L"RemoveNode")
     {
         intent = NodeDeactivationIntent::RemoveNode;
     }
     else
     {
         TestSession::FailTest("Invalid NodeDeactivationIntent");
     }

     return intent;
 }

 NodeDeactivationStatus::Enum FabricTestDispatcher::ParseNodeDeactivationStatus(std::wstring const& statusString)
 {
     NodeDeactivationStatus::Enum status = NodeDeactivationStatus::None;

     if (statusString == L"None")
     {
         status = NodeDeactivationStatus::None;
     }
     else if (statusString == L"DeactivationSafetyCheckInProgress")
     {
         status = NodeDeactivationStatus::DeactivationSafetyCheckInProgress;
     }
     else if (statusString == L"DeactivationSafetyCheckComplete")
     {
         status = NodeDeactivationStatus::DeactivationSafetyCheckComplete;
     }
     else if (statusString == L"DeactivationComplete")
     {
         status = NodeDeactivationStatus::DeactivationComplete;
     }
     else if (statusString == L"ActivationInProgress")
     {
         status = NodeDeactivationStatus::ActivationInProgress;
     }
     else
     {
         TestSession::FailTest("Invalid NodeDeactivationStatus");
     }

     return status;
 }

bool FabricTestDispatcher::DeactivateNodes(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeactivateNodesCommand);
        return false;
    }

    wstring batchId = params[0];

    map<NodeId, NodeDeactivationIntent::Enum> nodesToDeactivate;

    StringCollection pairs;
    StringUtility::Split<wstring>(params[1], pairs, L",");
    for (auto iter = pairs.begin(); iter != pairs.end(); iter++)
    {
        StringCollection pair;
        StringUtility::Split<wstring>(*iter, pair, L":");
        TestSession::FailTestIfNot(pair.size() == 2, "Invalid format for arg {0}", params[1]);

        NodeId nodeId = ParseNodeId(pair[0]);
        NodeDeactivationIntent::Enum intent = ParseNodeDeactivationIntent(pair[1]);

        nodesToDeactivate[nodeId] = intent;
    }

    CommandLineParser parser(params, 2);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");

    ErrorCodeValue::Enum expectedError = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);

    ErrorCode actualError;
    TimeoutHelper timeoutHelper(TimeSpan::FromSeconds(60.0));

    do
    {
        FabricTestNodeSPtr testNode = Federation.GetNodeClosestToZero();

        testNode->Node->GetReliabilitySubsystemUnderLock(
            [this, &nodesToDeactivate, &batchId, &actualError, &expectedError](ReliabilityTestApi::ReliabilityTestHelper & reliabilityTestHelper)
            {
                auto & reliability = reliabilityTestHelper.Reliability;

                ManualResetEvent manualResetEvent;
                auto operation = reliability.AdminClient.BeginDeactivateNodes(
                    move(nodesToDeactivate),
                    batchId,
                    FabricActivityHeader(Guid::NewGuid()),
                    TimeSpan::FromSeconds(60.0),
                    [&manualResetEvent](AsyncOperationSPtr const&) { manualResetEvent.Set(); },
                    AsyncOperationSPtr());

                manualResetEvent.WaitOne();

                actualError = reliability.AdminClient.EndDeactivateNodes(operation);
            });

        if (!actualError.IsError(expectedError))
        {
            TestSession::WriteInfo(TraceSource, "DeactivateNodes failed. Expected={0}, Actual={1}. Retrying...", expectedError, actualError);
            TestSession::FailTestIf(timeoutHelper.IsExpired, "DeactivateNodes failed. Expected={0}, Actual={1}.", expectedError, actualError);
        }
    } while (!timeoutHelper.IsExpired && !actualError.IsError(expectedError));

    return true;
}

bool FabricTestDispatcher::RemoveNodeDeactivation(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveNodeDeactivationCommand);
        return false;
    }

    wstring batchId = params[0];

    CommandLineParser parser(params, 1);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");

    ErrorCodeValue::Enum expectedError = FABRICSESSION.FabricDispatcher.ParseErrorCode(errorString);

    ErrorCode actualError;
    TimeoutHelper timeoutHelper(TimeSpan::FromSeconds(60.0));

    do
    {
        FabricTestNodeSPtr testNode = Federation.GetNodeClosestToZero();

        testNode->Node->GetReliabilitySubsystemUnderLock(
            [this, &batchId, &actualError, &expectedError](ReliabilityTestApi::ReliabilityTestHelper & reliabilityTestHelper)
            {
                auto & reliability = reliabilityTestHelper.Reliability;

                ManualResetEvent manualResetEvent;

                auto operation = reliability.AdminClient.BeginRemoveNodeDeactivations(
                    batchId,
                    FabricActivityHeader(Guid::NewGuid()),
                    TimeSpan::FromSeconds(60.0),
                    [&manualResetEvent](AsyncOperationSPtr const&) { manualResetEvent.Set(); },
                    AsyncOperationSPtr());

                manualResetEvent.WaitOne();

                actualError = reliability.AdminClient.EndRemoveNodeDeactivations(operation);
            });

        if (!actualError.IsError(expectedError))
        {
            TestSession::WriteInfo(TraceSource, "RemoveNodeDeactivation failed. Expected={0}, Actual={1}. Retrying...", expectedError, actualError);
            TestSession::FailTestIf(timeoutHelper.IsExpired, "RemoveNodeDeactivation failed. Expected={0}, Actual={1}.", expectedError, actualError);
        }
    } while (!timeoutHelper.IsExpired && !actualError.IsError(expectedError));

    return true;
}

bool FabricTestDispatcher::VerifyNodeDeactivationStatus(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyNodeDeactivationStatusCommand);
        return false;
    }

    wstring batchId = params[0];

    NodeDeactivationStatus::Enum expectedStatus = ParseNodeDeactivationStatus(params[1]);

    TimeSpan timeout;
    CommandLineParser parser(params, 2);
    double seconds;
    if (parser.TryGetDouble(L"timeout", seconds))
    {
        timeout = TimeSpan::FromSeconds(seconds);
    }
    else
    {
        timeout = FabricTestSessionConfig::GetConfig().VerifyTimeout;
    }

    TimeoutHelper helper(timeout);
    bool done = false;
    while (!done)
    {
        FabricTestNodeSPtr testNode = Federation.GetNodeClosestToZero();
        NodeDeactivationStatus::Enum status = NodeDeactivationStatus::None;
        vector<NodeProgress> progressDetails;

        testNode->Node->GetReliabilitySubsystemUnderLock(
            [this, &batchId, &status, &progressDetails](ReliabilityTestHelper & reliabilityTestHelper)
            {
                auto & reliability = reliabilityTestHelper.Reliability;

                ManualResetEvent manualResetEvent;

                auto operation = reliability.AdminClient.BeginGetNodeDeactivationStatus(
                    batchId,
                    FabricActivityHeader(Guid::NewGuid()),
                    TimeSpan::FromSeconds(60.0),
                    [&manualResetEvent](AsyncOperationSPtr const&) { manualResetEvent.Set(); },
                    AsyncOperationSPtr());

                manualResetEvent.WaitOne();

                ErrorCode error = reliability.AdminClient.EndGetNodeDeactivationsStatus(operation, status, progressDetails);

                TestSession::FailTestIfNot(error.IsSuccess(), "NodeDeactivationStatus failed with unexpected error {0}", error);
            });

        if (status != expectedStatus)
        {
            TestSession::WriteInfo(TraceSource, "NodeDeactivationStatus failed. Expected {0} Actual {1}. Retrying...\r\n{2}", expectedStatus, status, progressDetails);
            TestSession::FailTestIf(helper.IsExpired, "Timeout verifying NodeDeactivationStatus");
            Sleep(2000);
        }
        else
        {
            done = true;
        }
    }

    return true;
}

bool FabricTestDispatcher::KillCodePackage(Common::StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::KillCodePackageCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);

    CommandLineParser parser(params, 1);
    wstring codePackageIdentifier;
    parser.TryGetString(L"cp", codePackageIdentifier, L"");
    bool killDllHost = parser.GetBool(L"killhost");

    FabricTestNodeSPtr testNode = testFederation_.GetNode(nodeId);
    if (!testNode)
    {
        TestSession::WriteWarning(TraceSource, "KillCodePackage for invalid node {0}", nodeId);
        return false;
    }

    if(killDllHost)
    {
        return testNode->KillHostAt(codePackageIdentifier);
    }
    else
    {
        return testNode->KillCodePackage(codePackageIdentifier);
    }
}

bool FabricTestDispatcher::SetCodePackageKillPending(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::SetCodePackageKillPendingCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    wstring codePackageIdentifier = params[1];

    FabricTestNodeSPtr testNode = testFederation_.GetNode(nodeId);
    if (!testNode)
    {
        TestSession::WriteWarning(TraceSource, "KillCodePackage for invalid node {0}", nodeId);
        return false;
    }

    return testNode->MarkKillPending(codePackageIdentifier);
}

bool FabricTestDispatcher::KillFailoverManagerService(StringCollection const & params)
{
    if (params.size() > 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::KillFailoverManagerServiceCommand);
        return false;
    }

    StringCollection mergedParams;
    mergedParams.push_back(ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName);

    for (auto iter = params.begin(); iter != params.end(); ++iter)
    {
        mergedParams.push_back(move(*iter));
    }

    return KillService(mergedParams, GetFMM().get());
}

bool FabricTestDispatcher::KillNamingService(StringCollection const & params)
{
    if (params.size() > 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::KillNamingServiceCommand);
        return false;
    }

    StringCollection mergedParams;
    mergedParams.push_back(ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName);

    for (auto iter = params.begin(); iter != params.end(); ++iter)
    {
        mergedParams.push_back(move(*iter));
    }

    return KillService(mergedParams);
}

void DeleteDirectoryWithRetry(wstring const & directory)
{
    if (Directory::Exists(directory))
    {
        ErrorCode error;
        int maxAttempts = 24;
    
        for (auto attempt=0; attempt<maxAttempts; ++attempt)
        {
            error = Directory::Delete(directory, true, true);
            if (error.IsSuccess())
            {
                return;
            }
            else if (attempt < maxAttempts-1)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Cannot delete directory {0} due to {1}: retrying...", directory, error);

                Sleep(5000);
            }
        }

        TestSession::FailTestIfNot(error.IsSuccess(), "Cannot delete directory {0} due to {1}", directory, error);
    }
}

void CreateDirectoryWithRetry(wstring const & directory)
{
    int retryCount = 0;
    int maxRetryCount = 5;
    bool done = false;

    ErrorCode error;
    while(!done)
    {
        error = Directory::Create2(FabricTestDispatcher::TestDataDirectory);
        if(!error.IsSuccess() && retryCount >= maxRetryCount)
        {
            Sleep(5000);
            ++retryCount;
        }
        else
        {
            done = true;
        }
    }

    TestSession::FailTestIfNot(error.IsSuccess(), "Cannot create directory {0} due to {1}", directory, error);
}

bool FabricTestDispatcher::KillService(
    StringCollection const & params,
    FailoverManagerTestHelper * fmPtr)
{
    if (params.size() < 1 || params.size() > 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::KillServiceCommand);
        return false;
    }

    wstring const & serviceName = params[0];

    CommandLineParser parser(params, 1);
    bool clearDb = parser.GetBool(L"cleardb");

    bool primaryOnly = parser.GetBool(L"primaryOnly");

    bool secondariesOnly = parser.GetBool(L"secondariesOnly");

    set<NodeId> nodesToKill;
    {
        FailoverManagerTestHelperUPtr fm;
        if (!fmPtr)
        {
            fm = GetFM();
            fmPtr = fm.get();
        }

        TestSession::FailTestIf(fmPtr == nullptr, "FM is not ready");

        vector<FailoverUnitSnapshot> serviceFUs = fmPtr->FindFTByServiceName(serviceName);

        for (auto fuIter = serviceFUs.begin(); fuIter != serviceFUs.end(); ++fuIter)
        {
            auto replicas = fuIter->GetReplicas();
            for (auto replicaIter = replicas.cbegin(); replicaIter != replicas.cend(); ++replicaIter)
            {
                bool shouldKill = true;

                if (primaryOnly && !replicaIter->IsCurrentConfigurationPrimary ||
                    secondariesOnly && !replicaIter->IsCurrentConfigurationSecondary)
                {
                    shouldKill = false;
                }

                if (shouldKill)
                {
                    nodesToKill.insert(replicaIter->FederationNodeId);
                }
            }
        }
    }

    for (auto nodeIter = nodesToKill.begin(); nodeIter != nodesToKill.end(); ++nodeIter)
    {
        TestSession::WriteInfo(TraceSource, "Removing node: {0}", *nodeIter);

        if (!RemoveNode(nodeIter->ToString(), clearDb))
        {
            return false;
        }
    }

    if (clearDb)
    {
        auto nodesDir = Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestNodesDirectory);

        for (auto iter = nodesToKill.begin(); iter != nodesToKill.end(); ++iter)
        {
            auto workingDir = Path::Combine(nodesDir, iter->ToString());

            wstring dbDir;

            if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName)
            {
                dbDir = Path::Combine(workingDir, FailoverConfig::GetConfig().FMStoreDirectory);
            }
            else if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName)
            {
                dbDir = Path::Combine(workingDir, Naming::Constants::NamingStoreDirectory);
            }
            else if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName)
            {
                dbDir = Path::Combine(workingDir, Management::ClusterManager::Constants::DatabaseDirectory);
            }

            if (!dbDir.empty())
            {
                DeleteDirectoryWithRetry(dbDir);
            }
        }
    }

    return true;
}

bool FabricTestDispatcher::InjectError(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::InjectErrorCommand);
        return false;
    }

    wstring tag = params[0];

    ErrorCodeValue::Enum error = ParseErrorCode(params[1]);

    CommandLineParser parser(params, 2);

    int expiration;
    parser.TryGetInt(L"expiration", expiration, 0);

    FaultInjector::GetGlobalFaultInjector().InjectTransientError(tag, error, TimeSpan::FromSeconds(expiration));

    return true;
}

bool FabricTestDispatcher::RemoveError(StringCollection const & params)
{
    if (params.size() != 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveErrorCommand);
        return false;
    }

    wstring tag = params[0];

    FaultInjector::GetGlobalFaultInjector().RemoveError(tag);

    return true;
}

bool FabricTestDispatcher::OpenHttpGateway(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::HttpGateway);
        return false;
    }

    if (params[0] == L"close")
    {
        this->CloseHttpGateway();
        return true;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    FabricTestNodeSPtr node = testFederation_.GetNode(nodeId);
    if (!node)
    {
        TestSession::FailTest("Node {0} does not exist", nodeId);
    }

    shared_ptr<FabricNodeWrapper> fabricNodeWrapper;
    TestSession::FailTestIfNot(node->TryGetFabricNodeWrapper(fabricNodeWrapper), "HttpGatewayCommand for node id {0} shold be for an in proc node", nodeId);

    auto configSPtr = fabricNodeWrapper->Config;

    CommandLineParser parser(params, 1);

    int port;
    if (parser.TryGetInt(L"port", port, 0))
    {
        wstring httpListenAdress = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(port));
        wstring httpProtocol = L"http";

        configSPtr->HttpGatewayListenAddress = httpListenAdress;
        configSPtr->HttpGatewayProtocol = httpProtocol;
    }

    this->CloseHttpGateway();

    TestSession::WriteInfo(TraceSource, "Opening HttpGateway at {0}", fabricNodeWrapper->Config->HttpGatewayListenAddress);

    ManualResetEvent wait(false);
    auto operation = HttpGateway::HttpGatewayImpl::BeginCreateAndOpen(
        move(configSPtr),
        [&wait] (AsyncOperationSPtr const &) { wait.Set(); });

    wait.WaitOne();

    auto error = HttpGateway::HttpGatewayImpl::EndCreateAndOpen(operation, httpGateway_);
    TestSession::FailTestIfNot(error.IsSuccess(), "Failed to open HttpGateway: error={0}", error);

    return true;
}

void FabricTestDispatcher::CloseHttpGateway()
{
    if (httpGateway_)
    {
        TestSession::WriteInfo(TraceSource, "Closing HttpGateway");

        ManualResetEvent wait(false);

        auto operation = httpGateway_->BeginClose(
            FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
            [&wait] (AsyncOperationSPtr const &) { wait.Set(); });

        wait.WaitOne();

        auto error = httpGateway_->EndClose(operation);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to close HttpGateway: error={0}", error);
    }
}

//
// Deserialization helpers for esedump command
//

typedef std::function<std::wstring(std::vector<byte> &)> EseDumpDeserializationCallback;
std::map<std::wstring, EseDumpDeserializationCallback> EseDumpTypeDeserializers;
std::map<std::wstring, EseDumpDeserializationCallback> EseDumpKeyDeserializers;

std::wstring EseDumpReplicaHealthEvent(__in vector<byte> & bytes)
{
    EntityHealthEventStoreData<ReplicaHealthId> replicaHealth;
    auto error = FabricSerializer::Deserialize(replicaHealth, bytes);

    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "Failed to deserialize {0}: error={1}",
        replicaHealth.Type,
        error);

    return wformatString(
        "modified={0} ttl={1} remove.expired={2}",
        replicaHealth.LastModifiedUtc,
        replicaHealth.TimeToLive,
        replicaHealth.RemoveWhenExpired);
}

std::wstring EseDumpReplicaHealthAttribute(__in vector<byte> & bytes)
{
    ReplicaAttributesStoreData replicaAttr;
    auto error = FabricSerializer::Deserialize(replicaAttr, bytes);

    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "Failed to deserialize {0}: error={1}",
        replicaAttr.Type,
        error);

    return wformatString(
        "attr.flags={0:0X}",
        replicaAttr.AttributeSetFlags.Value);
}

std::wstring EseDumpEpochHistory(__in vector<byte> & bytes)
{
    ProgressVectorData epochHistory;
    auto error = FabricSerializer::Deserialize(epochHistory, bytes);

    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "Failed to deserialize {0}: error={1}",
        Store::Constants::EpochHistoryDataName,
        error);

    return wformatString(
        "history={0}",
        epochHistory);
}

std::wstring EseDumpCurrentEpoch(__in vector<byte> & bytes)
{
    CurrentEpochData currentEpoch;
    auto error = FabricSerializer::Deserialize(currentEpoch, bytes);

    TestSession::FailTestIfNot(
        error.IsSuccess(),
        "Failed to deserialize {0}: error={1}",
        Store::Constants::CurrentEpochDataName,
        error);

    return wformatString(
        "current={0}",
        currentEpoch);
}

map<wstring, EseDumpDeserializationCallback> EseDumpGetTypeDeserializers()
{
    map<wstring, EseDumpDeserializationCallback> deserializers;

    deserializers[Management::HealthManager::Constants::StoreType_ReplicaHealthEvent]
        = EseDumpReplicaHealthEvent;

    deserializers[Management::HealthManager::Constants::StoreType_ReplicaHealthAttributes]
        = EseDumpReplicaHealthAttribute;

    return deserializers;
}

map<wstring, EseDumpDeserializationCallback> EseDumpGetKeyDeserializers()
{
    map<wstring, EseDumpDeserializationCallback> deserializers;

    deserializers[Store::Constants::CurrentEpochDataName]
        = EseDumpCurrentEpoch;

    deserializers[Store::Constants::EpochHistoryDataName]
        = EseDumpEpochHistory;

    return deserializers;
}

bool FabricTestDispatcher::EseDump(StringCollection const & params)
{
#if defined(PLATFORM_UNIX)
    Assert::CodingError("Not supported");
#else
    if (EseDumpTypeDeserializers.empty())
    {
        EseDumpTypeDeserializers = EseDumpGetTypeDeserializers();
    }

    if (EseDumpKeyDeserializers.empty())
    {
        EseDumpKeyDeserializers = EseDumpGetKeyDeserializers();
    }

    CommandLineParser parser(params, 0);

    EseLocalStoreSettings settings;

    wstring directory;
    parser.TryGetString(L"dir", directory, L".");

    wstring filename;
    parser.TryGetString(L"file", filename, L"ese");

    wstring enumType;
    parser.TryGetString(L"type", enumType, L"");

    bool noLsnColumn = parser.GetBool(L"nolsn");

    LONG flags = EseLocalStore::LOCAL_STORE_FLAG_NONE;

    if (noLsnColumn)
    {
        flags &= ~EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN;
    }
    else
    {
        flags |= EseLocalStore::LOCAL_STORE_FLAG_USE_LSN_COLUMN;
    }

    settings.StoreName = filename;

    EseLocalStore eseStore(directory, settings, flags);

    auto error = eseStore.Initialize(L"EseDump");
    TestSession::FailTestIfNot(error.IsSuccess(), "Failed to initialize: error={0}", error);

    ILocalStore::TransactionSPtr tx;
    error = eseStore.CreateTransaction(tx);
    TestSession::FailTestIfNot(error.IsSuccess(), "Failed to create transaction: error={0}", error);

    ILocalStore::EnumerationSPtr en;
    if (!enumType.empty())
    {
        error = eseStore.CreateEnumerationByTypeAndKey(tx, enumType, L"", en);
    }
    else
    {
        error = eseStore.CreateEnumerationByOperationLSN(tx, 0, en);
    }
    TestSession::FailTestIfNot(error.IsSuccess(), "Failed to create enumeration: error={0}", error);

    int count = 0;
    size_t totalBytes = 0;
    size_t charCount = 0;

    while ((error = en->MoveNext()).IsSuccess())
    {
        wstring type;
        error = en->CurrentType(type);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to get type: error={0}", error);

        wstring key;
        error = en->CurrentKey(key);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to get key: error={0}", error);

        _int64 lsn;
        error = en->CurrentOperationLSN(lsn);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to get lsn: error={0}", error);

        vector<byte> value;
        error = en->CurrentValue(value);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to get value: error={0}", error);

        FILETIME filetime;
        error = en->CurrentLastModifiedFILETIME(filetime);
        TestSession::FailTestIfNot(error.IsSuccess(), "Failed to get filetime: error={0}", error);

        DateTime datetime(filetime);

        wstring deserializedDetails;
        auto it = EseDumpTypeDeserializers.find(type);
        if (it != EseDumpTypeDeserializers.end())
        {
            deserializedDetails = (it->second)(value);
        }
        else
        {
            auto dumpKeyIt = EseDumpKeyDeserializers.find(key);
            if (dumpKeyIt != EseDumpKeyDeserializers.end())
            {
                deserializedDetails = (dumpKeyIt->second)(value);
            }
        }

        TestSession::WriteInfo(TraceSource,
            "{0}:{1}, {2}:{3}, {4} bytes, {5}, [{6}]",
            count,
            lsn,
            type,
            key,
            value.size(),
            datetime,
            deserializedDetails);

        ++count;
        totalBytes += value.size();
        charCount += type.size() + key.size();
    }

    TestSession::FailTestIfNot(
        error.IsError(ErrorCodeValue::EnumerationCompleted),
        "Failed enumeration: error={0}", error);

    TestSession::WriteInfo(TraceSource,
        "Enumeration completed: count={0} value-bytes={1} key-bytes={2}",
        count,
        totalBytes,
        charCount * sizeof(wchar_t));

    return true;
#endif
}

bool FabricTestDispatcher::CreateNamedFabricClient(Common::StringCollection const & params)
{
    if (params.size() != 1 && params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CreateClientCommand);
        return false;
    }

    CommandLineParser parser(params, 1);
    wstring nodeIdString;
    ComPointer<IFabricServiceManagementClient> serviceClient;
    if (parser.TryGetString(L"nodeconfig", nodeIdString, L""))
    {
        CreateFabricClientOnNode(nodeIdString, /*out*/serviceClient);
    }
    else
    {
        StringCollection addresses;
        if (parser.GetVector(L"ipaddresses", addresses, L""))
        {
            CreateFabricClientAtAddresses(addresses, serviceClient);
        }
    }

    if (clientManager_.AddClient(params[0], move(serviceClient)))
    {
        TestSession::WriteInfo(TraceSource, "Fabric client {0} created", params[0]);
        return true;
    }
    else
    {
        return false;
    }
}

bool FabricTestDispatcher::DeleteNamedFabricClient(Common::StringCollection const & params)
{
    if (params.size() != 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DeleteClientCommand);
        return false;
    }

    if (clientManager_.DeleteClient(params[0]))
    {
        TestSession::WriteInfo(TraceSource, "Fabric client {0} deleted", params[0]);
        return true;
    }
    else
    {
        return false;
    }
}

bool FabricTestDispatcher::RegisterCallback(Common::StringCollection const & params)
{
    if (params.size() < 4 || params.size() > 5)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RegisterCallbackCommand);
        return false;
    }

    std::wstring clientName = params[0];
    std::wstring callbackName = params[1];
    std::wstring uriName = params[2];

    FABRIC_PARTITION_KEY_TYPE keyType;
    __int64 key;
    void const * keyPtr = NULL;
    if (IsFalse(params[3]))
    {
        keyType = FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE;
        keyPtr = NULL;
    }
    else if (Common::TryParseInt64(params[3], key))
    {
        keyType = FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64;
        keyPtr = reinterpret_cast<void const *>(&key);
    }
    else
    {
        keyType = FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING;
        keyPtr = reinterpret_cast<void const *>(params[3].c_str());
    }

    CommandLineParser parser(params, 1);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    HRESULT expectedError = ErrorCode(ParseErrorCode(errorString)).ToHResult();

    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(clientName, client))
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} does not exists.", clientName);
        return false;
    }

    HRESULT hr = client->AddCallback(callbackName, uriName, keyType, keyPtr);
    if (hr != expectedError)
    {
        TestSession::WriteInfo(TraceSource, "Callback {0} on client {1}: register failed with {2}, expected {3}", callbackName, clientName, hr, expectedError);
        return false;
    }

    if (SUCCEEDED(hr))
    {
        TestSession::WriteInfo(TraceSource, "Callback {0} is registered on client {1}", callbackName, clientName);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "Callback {0} on client {1}: register failed with {2}", callbackName, clientName, hr);
    }

    return true;
}

bool FabricTestDispatcher::UnregisterCallback(Common::StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UnregisterCallbackCommand);
        return false;
    }

    std::wstring clientName = params[0];
    std::wstring callbackName = params[1];

    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(clientName, client))
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} does not exists.", clientName);
        return false;
    }

    if (client->DeleteCallback(callbackName))
    {
        TestSession::WriteInfo(TraceSource, "Callback {0} is unregistered from client {1}", callbackName, clientName);
        return true;
    }
    else
    {
        return false;
    }
}

bool FabricTestDispatcher::WaitForCallback(Common::StringCollection const & params)
{
    if (params.size() < 2 || params.size() > 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::WaitForCallbackCommand);
        return false;
    }
    std::wstring clientName = params[0];
    std::wstring callbackName = params[1];

    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(clientName, client))
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} does not exists.", clientName);
        return false;
    }

    TimeSpan timeout;
    CommandLineParser parser(params, 1);
    double seconds;
    if (parser.TryGetDouble(L"timeout", seconds))
    {
        timeout = TimeSpan::FromSeconds(seconds);
    }
    else
    {
        timeout = FabricTestSessionConfig::GetConfig().VerifyTimeout;
    }

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    HRESULT expectedError = ErrorCode(ParseErrorCode(errorString)).ToHResult();

    return client->WaitForCallback(callbackName, timeout, expectedError);
}

bool FabricTestDispatcher::RemoveClientCacheItem(Common::StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveCachedServiceResolutionCommand);
        return false;
    }
    std::wstring clientName = params[0];
    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(clientName, client))
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} does not exists.", clientName);
        return false;
    }

    if (client->DeleteCacheItem(params[1]))
    {
        TestSession::WriteInfo(TraceSource, "Cached service resolution {0} in client {1} deleted", params[1], clientName);
        return true;
    }
    else
    {
        return false;
    }
}

bool FabricTestDispatcher::VerifyFabricTime(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyFabricTimeCommand);
        return false;
    }
    wstring serviceName(params[0]);
    NodeId nodeId = ParseNodeId(params[1]);
    CommandLineParser parser(params, 2);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum expectedError = ParseErrorCode(errorString);
    ITestStoreServiceSPtr testStoreService;
    TestSession::FailTestIfNot(
        FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName,nodeId,testStoreService),
        "Fail to get Service:{0} on Node:{1}",serviceName,nodeId);
    int64 timeInTicks;
    ErrorCode errorCode;
    errorCode = testStoreService->getCurrentStoreTime(timeInTicks);
    int retry = 20;
    while(errorCode.IsError(ErrorCodeValue::NotReady) && (--retry > 0))
    {
        //it is possible that the query it too quick when the replica become primary
        //and the fabricTimeController is not ready. (still loading value from replicated store)
        //it ususally not happen, if it happens, retry several times
        Sleep(400);
        errorCode = testStoreService->getCurrentStoreTime(timeInTicks);
    }
    //assert if the error code is not expected
    //the normal cases is that Test script will verify on secondary and the expectedError would be NotPrimary
    TestSession::WriteInfo(
        TraceSource,
        "Result from fabric timer: ErrorCode:{0} timeInTicks:{1}",errorCode,timeInTicks);
    TestSession::FailTestIfNot(errorCode.IsError(expectedError),"expecting Error {0}, actual {1}",expectedError,errorCode);

    //Logical time should always be forwarding, since we have the persistence interval (usualy 1 sec)
    //at most FabricTimeInterval backwarding can happen if primary goes down and the time is
    //not persisted
    if(errorCode.IsSuccess())
    {
        TestSession::FailTestIfNot(
            timeInTicks>FABRICSESSION.LastGotLogicalTime-Store::StoreConfig().FabricTimePersistInterval.Ticks,
            "Logic Time Error. Previous recorded time {0}, current time {1}",
            FABRICSESSION.LastGotLogicalTime,timeInTicks);
        FABRICSESSION.LastGotLogicalTime=timeInTicks;
    }

    return true;
}

bool FabricTestDispatcher::VerifyCacheItem(Common::StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyCachedServiceResolutionCommand);
        return false;
    }

    std::wstring clientName = params[0];
    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(clientName, client))
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} does not exist.", clientName);
        return false;
    }

    std::wstring cacheItemName = params[1];
    ServiceLocationChangeCacheInfoSPtr cacheInfo;
    if (!client->GetCacheInfo(cacheItemName, cacheInfo))
    {
        TestSession::WriteError(TraceSource, "Cache with name: {0} does not exist.", cacheItemName);
        return false;
    }

    ResolvedServicePartition resolvedServiceLocations;
    HRESULT hr = TryConvertComResult(cacheInfo->CacheItem, resolvedServiceLocations);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "Converting COM service results failed. hresult = {0}", hr);

    bool expected = IsFalse(params[2]) ? false : true;
    TimeoutHelper timeoutHelper(FabricTestSessionConfig::GetConfig().VerifyTimeout);
    for (;;)
    {
        bool verifyResult = FABRICSESSION.FabricDispatcher.VerifyResolveResult(resolvedServiceLocations, cacheInfo->ServiceName) ? true : false;
        if (verifyResult == expected) { break; }
        TestSession::WriteError(TraceSource, "verifycachedserviceresolution expected {0}, actual {1}", expected, verifyResult);
        if (timeoutHelper.IsExpired) { return false; }
        Sleep(1000);
    }
    TestSession::WriteInfo(TraceSource, "cached service resolution {0} in client {1} is in expected state: {2}", cacheItemName, clientName, expected);
    return true;
}

bool FabricTestDispatcher::AddNode(Common::StringCollection const & params)
{
    wstring nodeIdStr = params[0];
    CommandLineParser parser(params, 1);
    bool expectFailure = parser.GetBool(L"fail");
    bool skipRuntime = parser.GetBool(L"skipruntime");
    StringCollection nodeCredentials;
    parser.GetVector(L"security", nodeCredentials, defaultNodeCredentials_, false);

    if((clusterWideCredentials_.size() > 0 && nodeCredentials.size() == 0 && StringUtility::CompareCaseInsensitive(clusterWideCredentials_.front(), L"Windows")) ||
        (clusterWideCredentials_.size() == 0 && nodeCredentials.size() != 0))
    {
        TestSession::WriteError(TraceSource, "Invalid security settings.");
        return false;
    }

    wstring openErrorCodeString;
    parser.TryGetString(L"error", openErrorCodeString, L"Success");
    ErrorCodeValue::Enum openErrorCode = ParseErrorCode(openErrorCodeString);

    StringCollection retryErrorStrings;
    parser.GetVector(L"retryerrors", retryErrorStrings, L"");
    vector<ErrorCodeValue::Enum> retryErrors;
    for (auto it = retryErrorStrings.begin(); it != retryErrorStrings.end(); ++it)
    {
        retryErrors.push_back(ParseErrorCode(*it));
    }

    wstring nodeUpgradeDomainId;
    parser.TryGetString(L"ud", nodeUpgradeDomainId, L"");

    wstring nodeFaultDomainId;
    parser.TryGetString(L"fd", nodeFaultDomainId, L"");

    wstring systemServiceInitializationTimeout;
    parser.TryGetString(L"SystemServiceInitializationTimeout", systemServiceInitializationTimeout, L"1");

    wstring systemServiceInitializationRetryInterval;
    parser.TryGetString(L"SystemServiceInitializationRetryInterval", systemServiceInitializationRetryInterval, L"1");

    Common::Uri nodeFaultDomainIdUri;
    if (!nodeFaultDomainId.empty() && !Common::Uri::TryParse(nodeFaultDomainId, nodeFaultDomainIdUri))
    {
        TestSession::WriteError(
            TraceSource,
            "Incorrect uri for fault domain id {0}",
            nodeFaultDomainId);
        return false;
    }

    map<wstring,wstring> nodeProperties;
    parser.GetMap(L"nodeprops", nodeProperties);

    map<wstring,wstring> nodeCapacities;
    vector<wstring> nodeCapacityVec;
    parser.GetVector(L"cap", nodeCapacityVec);

    // Handle case when metric can have ":" in the name
    for (auto token : nodeCapacityVec)
    {
        Common::StringCollection capCollection;
        Common::StringUtility::Split<std::wstring>(token, capCollection, L":");
        wstring capacity = capCollection.back();
        TestSession::FailTestIfNot(capCollection.size() >= 2, "Invalid format for arg {0}", token);
        wstring metricName = capCollection[0];
        for (size_t index = 1; index < capCollection.size() - 1; ++index)
        {
            metricName += L":";
            metricName += capCollection[index];
        }
        nodeCapacities.insert(make_pair(metricName, capacity));
    }

    bool idempotent = parser.GetBool(L"idempotent");

    NodeId nodeId = ParseNodeId(nodeIdStr);

    if (testFederation_.ContainsNode(nodeId))
    {
        if (idempotent)
        {
            TestSession::WriteInfo(TraceSource, "SiteNode with the same Id: {0} already exists.", params);
            return true;
        }
        else
        {
            TestSession::WriteError(TraceSource, "SiteNode with the same Id: {0} already exists.", params);
            return false;
        }
    }

    FabricVersionInstance fabVerInst;
    if (parser.TryGetFabricVersionInstance(L"version", fabVerInst))
    {
        FABRICSESSION.FabricDispatcher.UpgradeFabricData.SetFabricVersionInstance(nodeId, fabVerInst);
    }

    // Override FabricTestCommonConfig setting values before creating
    // FabricTestNode instance, which loads some of these settings
    // into environment variables for activated service hosts to read.
    //
    if (openErrorCode == ErrorCodeValue::Success && Federation.Count == 0)
    {
        // First node to be added. Setup the system service descriptions
        //
        SetSharedLogSettings();
        SetSystemServiceStorageProvider();
        SetSystemServiceDescriptions();
    }

    FabricNodeConfigOverride configOverride(
        move(nodeUpgradeDomainId),
        move(nodeFaultDomainIdUri),
        move(systemServiceInitializationTimeout),
        move(systemServiceInitializationRetryInterval));

    shared_ptr<FabricTestNode> testNode = make_shared<FabricTestNode>(
        nodeId,
        configOverride,
        FabricNodeConfig::NodePropertyCollectionMap::Parse(nodeProperties),
        FabricNodeConfig::NodeCapacityCollectionMap::Parse(nodeCapacities),
        nodeCredentials,
        clusterWideCredentials_,
        openErrorCode == ErrorCodeValue::Success);

    if (openErrorCode == ErrorCodeValue::Success)
    {
        testFederation_.AddNode(testNode);
        ServiceMap.RecalculateBlockList();
    }

    testNode->Open(FabricTestSessionConfig::GetConfig().OpenTimeout, expectFailure, !skipRuntime, openErrorCode, retryErrors);
    return true;
}

bool FabricTestDispatcher::CloseAllNodes(bool abortNodes)
{
    auto votes = FederationConfig::GetConfig().Votes;
    vector<wstring> nonSeeds;
    vector<wstring> seeds;
    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        NodeId id = node->Id;
        auto it = votes.find(id);
        if (it == votes.end() || it->Type != Federation::Constants::SeedNodeVoteType)
        {
            nonSeeds.push_back(id.ToString());
        }
        else
        {
            seeds.push_back(id.ToString());
        }
    }

    for (auto iterator = nonSeeds.begin(); iterator != nonSeeds.end(); iterator++)
    {
        bool result = abortNodes ? AbortNode(*iterator) : RemoveNode(*iterator);
        if (!result)
        {
            return result;
        }
    }

    for (auto iterator = seeds.begin(); iterator != seeds.end(); iterator++)
    {
        bool result = abortNodes ? AbortNode(*iterator) : RemoveNode(*iterator);
        if (!result)
        {
            return result;
        }
    }
    return true;
}

bool FabricTestDispatcher::RemoveNode(wstring const & nodeId, bool removeData)
{
    StringCollection params;
    params.push_back(nodeId);

    if (removeData)
    {
        params.push_back(L"removedata");
    }
    
    return RemoveNode(params);
}

bool FabricTestDispatcher::RemoveNode(StringCollection const & params)
{
    auto nodeIdString = params[0];

    if (nodeIdString == L"*")
    {
        return CloseAllNodes(false/*Close nodes not abort*/);
    }
    else
    {
        CommandLineParser parser(params, 1);
        bool removeData = parser.GetBool(FabricTestCommands::RemoveDataParameter);

        NodeId nodeId = ParseNodeId(nodeIdString);
        shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteError(
                TraceSource,
                "Node {0} does not exist.", nodeId);
            return false;
        }

        testFederation_.RemoveNode(nodeId);
        testNode->Close(removeData);
        ServiceMap.RecalculateBlockList();
        TestSession::WriteInfo(TraceSource, "FabricTestNode with the Id: {0} succesfully closed.", nodeId);
        return true;
    }
}

bool FabricTestDispatcher::AbortNode(wstring const & params)
{
    if (params == L"*")
    {
        return CloseAllNodes(true/*abort nodes*/);
    }
    else
    {
        NodeId nodeId = ParseNodeId(params);
        shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteError(
                TraceSource,
                "Node {0} does not exist.", nodeId);
            return false;
        }

        testFederation_.RemoveNode(nodeId);
        testNode->Abort();
        ServiceMap.RecalculateBlockList();
        TestSession::WriteInfo(TraceSource, "FabricTestNode with the Id: {0} succesfully Aborted.", params);
        return true;
    }
}

bool FabricTestDispatcher::AddRuntime(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::AddRuntimeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    bool isStateful = IsTrue(params[1]);
    vector<wstring> serviceTypes;
    if (params.size() > 2)
    {
        serviceTypes = vector<wstring>(params.begin() + 2, params.end());
    }

    shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);

    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node {0} does not exist.", nodeId);
        return false;
    }

    if (!isStateful)
    {
        testNode->RuntimeManager->AddStatelessFabricRuntime(serviceTypes);
        return true;
    }
    else
    {
        testNode->RuntimeManager->AddStatefulFabricRuntime(serviceTypes);
        return true;
    }
}

bool FabricTestDispatcher::UnregisterRuntime(StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UnregisterRuntimeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    bool isStateful = IsTrue(params[1]);

    shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);

    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node {0} does not exist.", nodeId);
        return false;
    }

    if (!isStateful)
    {
        testNode->RuntimeManager->UnregisterStatelessFabricRuntime();
        return true;
    }
    else
    {
        testNode->RuntimeManager->UnregisterStatefulFabricRuntime();
        return true;
    }
}

bool FabricTestDispatcher::RemoveRuntime(StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveRuntimeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    bool isStateful = IsTrue(params[1]);

    shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);

    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node {0} does not exist.", nodeId);
        return false;
    }

    if (!isStateful)
    {
        testNode->RuntimeManager->RemoveStatelessFabricRuntime();
        return true;
    }
    else
    {
        testNode->RuntimeManager->RemoveStatefulFabricRuntime();
        return true;
    }
}

bool FabricTestDispatcher::RegisterServiceType(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RegisterServiceTypeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    bool isStateful = IsTrue(params[1]);
    vector<wstring> serviceTypes(params.begin() + 2, params.end());
    shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);

    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node {0} does not exist.", nodeId);
        return false;
    }

    if (!isStateful)
    {
        testNode->RuntimeManager->RegisterStatelessServiceType(serviceTypes);
        return true;
    }
    else
    {
        testNode->RuntimeManager->RegisterStatefulServiceType(serviceTypes);
        return true;
    }
}

bool FabricTestDispatcher::EnableServiceType(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::EnableServiceTypeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);

    vector<wstring> serviceTypes(params.begin() + 1, params.end());
    shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);

    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node {0} does not exist.", nodeId);
        return false;
    }

    ServiceModel::ServiceTypeIdentifier serviceTypeId;
    ErrorCode error;
    for (auto it = serviceTypes.begin(); it != serviceTypes.end(); it++)
    {
        error = ServiceModel::ServiceTypeIdentifier::FromString(*it, serviceTypeId);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource,
                "Failed to convert ServiceType {0} from string to ServiceTypeIdentifier.", *it);
            return false;
        }

        testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
        {
            auto raTestHelper = reliabilityTestHelper.GetRA();
            raTestHelper->EnableServiceType(serviceTypeId, DateTime::Now().Ticks);
        });
    }

    return true;
}

bool FabricTestDispatcher::DisableServiceType(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DisableServiceTypeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);

    vector<wstring> serviceTypes(params.begin() + 1, params.end());
    shared_ptr<FabricTestNode> testNode = testFederation_.GetNode(nodeId);

    if (!testNode)
    {
        TestSession::WriteError(
            TraceSource,
            "Node {0} does not exist.", nodeId);
        return false;
    }

    ServiceModel::ServiceTypeIdentifier serviceTypeId;
    ErrorCode error;
    for (auto it = serviceTypes.begin(); it != serviceTypes.end(); it++)
    {
        error = ServiceModel::ServiceTypeIdentifier::FromString(*it, serviceTypeId);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(
                TraceSource,
                "Failed to convert ServiceType {0} from string to ServiceTypeIdentifier.", *it);
            return false;
        }

        testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
        {
            auto raTestHelper = reliabilityTestHelper.GetRA();
            raTestHelper->DisableServiceType(serviceTypeId, DateTime::Now().Ticks);
        });
    }

    return true;
}
TestStoreClientSPtr FabricTestDispatcher::GetTestClient(std::wstring const& serviceName)
{
    TestSession::FailTestIfNot(ServiceMap.IsStatefulService(serviceName), "Service {0} is not stateful", serviceName);
    if (testClients_.find(serviceName) == testClients_.end())
    {
        testClients_[serviceName] = make_shared<TestStoreClient>(serviceName);
    }

    return testClients_[serviceName];
}

void FabricTestDispatcher::ResetTestStoreClient(std::wstring const& serviceName)
{
    if (testClients_.find(serviceName) != testClients_.end())
    {
        testClients_[serviceName]->Reset();
    }
}

bool FabricTestDispatcher::SetSecondaryPumpEnabled(StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::SetSecondaryPumpEnabledCommand);
        return false;
    }

    wstring serviceName = params[0];
    NodeId nodeId = ParseNodeId(params[1]);

    bool enabled = IsTrue(params[2]);

    ITestStoreServiceSPtr storeService;

    bool found = false;
    int retry = 20;
    do
    {
        found = FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName, nodeId, storeService);
        Sleep(500);
    } while (!found && retry-- > 0);

    if (!found)
    {
        TestSession::WriteError(
            TraceSource,
            "Failed to find service {0} on node {1}.", serviceName, params[1]);
        return false;
    }

    storeService->SetSecondaryPumpEnabled(enabled);
    return true;
}

void FabricTestDispatcher::FindAndInvoke(
    Federation::NodeId const & nodeId,
    std::wstring const & serviceName,
    std::function<void (ITestStoreServiceSPtr const &)> storeServiceFunc,
    std::function<void (CalculatorServiceSPtr const &)> calculatorServiceFunc)
{
    ITestStoreServiceSPtr storeService;
    CalculatorServiceSPtr calculatorService;
    if (!FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName, nodeId, storeService))
    {
        if(!FABRICSESSION.FabricDispatcher.Federation.TryFindCalculatorService(serviceName, nodeId, calculatorService))
        {
            TestSession::WriteWarning(
                TraceSource,
                "Failed to find service {0} on node {1}.", serviceName, nodeId);
            return;
        }
    }

    if (storeService != nullptr && storeServiceFunc != nullptr)
    {
        storeServiceFunc(storeService);
    }

    if (calculatorService != nullptr && calculatorServiceFunc != nullptr)
    {
        calculatorServiceFunc(calculatorService);
    }
}

bool FabricTestDispatcher::ReportFault(StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ReportFaultCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    wstring serviceName = params[1];

    if (params[2].compare(L"permanent") != 0 && params[2].compare(L"transient") != 0)
    {
        TestSession::WriteError(
            TraceSource,
            "Unknown fault type {0}.", params[2]);
        return false;
    }

    ::FABRIC_FAULT_TYPE faultType =
        params[2].compare(L"permanent") == 0 ? ::FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_PERMANENT : ::FABRIC_FAULT_TYPE::FABRIC_FAULT_TYPE_TRANSIENT;

    FindAndInvoke(
        nodeId,
        serviceName,
        [faultType] (ITestStoreServiceSPtr const & storeService) { storeService->ReportFault(faultType); },
        [faultType] (CalculatorServiceSPtr const & calculatorService) { calculatorService->ReportFault(faultType); });

    return true;
}

bool FabricTestDispatcher::ReportLoad(StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ReportLoadCommand);
        return false;
    }

    wstring serviceName = params[0];
    NodeId nodeId = ParseNodeId(params[1]);

    StringCollection loadCollection;
    StringUtility::Split<wstring>(params[2], loadCollection, FabricTestDispatcher::ItemDelimiter);

    int count = static_cast<int>(loadCollection.size());

    ScopedHeap heap;
    FABRIC_LOAD_METRIC *metrics = heap.AddArray<FABRIC_LOAD_METRIC>(count).GetRawArray();

    for (int i = 0; i < count; i++)
    {
        StringCollection loadPair;
        StringUtility::Split<wstring>(loadCollection[i], loadPair, FabricTestDispatcher::KeyValueDelimiter);

        if (loadPair.size() != 2)
        {
            TestSession::WriteWarning(
                TraceSource,
                "Failed to parse load pair {0}", loadCollection[i]);
            return true;
        }

        metrics[i].Name = heap.AddString(loadPair[0]);
        metrics[i].Value = Common::Int32_Parse(loadPair[1]);
    }

    FindAndInvoke(
        nodeId,
        serviceName,
        [count, metrics] (ITestStoreServiceSPtr const & storeService) { storeService->ReportLoad(count, metrics); },
        [count, metrics] (CalculatorServiceSPtr const & calculatorService) { calculatorService->ReportLoad(count, metrics); });

    return true;
}

bool FabricTestDispatcher::ReportMoveCost(Common::StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ReportMoveCostCommand);
        return false;
    }

    wstring serviceName = params[0];
    NodeId nodeId = ParseNodeId(params[1]);
    uint moveCost = Common::Int32_Parse(params[2]);

    FindAndInvoke(
        nodeId,
        serviceName,
        [moveCost](ITestStoreServiceSPtr const & storeService) { storeService->ReportMoveCost((FABRIC_MOVE_COST)moveCost); },
        [moveCost](CalculatorServiceSPtr const & calculatorService) { calculatorService->ReportMoveCost((FABRIC_MOVE_COST)moveCost); });

    return true;
}

FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetPartitionAccessStatusFromString(wstring const & str)
{
    if (str == L"TryAgain")
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING;
    }
    else if (str == L"NotPrimary")
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY;
    }
    else if (str == L"NoWriteQuorum")
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM;
    }
    else if (str == L"Granted")
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
    }
    else
    {
        return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
    }
}

bool FabricTestDispatcher::VerifyReadWriteStatus(StringCollection const & params)
{
    if (params.size() != 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyReadWriteStatusCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    wstring serviceName = params[1];

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS expectedReadStatus = GetPartitionAccessStatusFromString(params[2]);
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS expectedWriteStatus = GetPartitionAccessStatusFromString(params[3]);

    if (expectedReadStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID || expectedWriteStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID)
    {
        TestSession::WriteError(
            TraceSource,
            "Unknown Read/Write Status {0} {1}", params[2], params[3]);
        return false;
    }

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS actualReadStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID, actualWriteStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;

    FindAndInvoke(
        nodeId,
        serviceName,
        [&] (ITestStoreServiceSPtr const & storeService)
        {
            actualReadStatus = storeService->GetReadStatus();
            actualWriteStatus = storeService->GetWriteStatus();
        },
        nullptr);

    TestSession::WriteInfo(TraceSource, "Retrieved Read/Write status for {0} on {1}. R = {2}. W= {3}", serviceName, nodeId, actualReadStatus, actualWriteStatus);

    if (expectedReadStatus != actualReadStatus || expectedWriteStatus != actualWriteStatus)
    {
        TestSession::FailTest(
            "Read and write status does not match. Read: Expected = {0} Actual = {1}. Write: Expected = {2} Actual = {3}",
            expectedReadStatus,
            actualReadStatus,
            expectedWriteStatus,
            actualWriteStatus);
    }

    return true;
}

bool FabricTestDispatcher::CheckIfLfupmIsEmpty(Common::StringCollection const& params)
{
    if (params.size() != 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CheckIfLFUPMIsEmptyCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);

    auto node = testFederation_.GetNode(nodeId);
    if (node == nullptr)
    {
        TestSession::WriteError(
            TraceSource,
            "Unknown Node {0}", nodeId);
        return false;
    }

    auto proxy = node->GetProxyForStatefulHost();

    auto fupIds = proxy->GetItemsInLFUPM();

    for (auto const & it : fupIds)
    {
        TestSession::WriteInfo("LFUPM", "{0}", it);
    }

    TestSession::FailTestIf(!fupIds.empty(), "FUP map is not empty on node {0}", nodeId);

    return true;
}

bool FabricTestDispatcher::CallService(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::CallService);
        return false;
    }

    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);
    if (!failoverUnit)
    {
        TestSession::WriteWarning(TraceSource, "Failed to find failover unit for {0}", params[0]);
    }

    std::wstring serviceName = failoverUnit->ServiceName;

    auto nodeId = ParseNodeId(params[1]);

    auto & command = params[2];

    ITestScriptableServiceSPtr target;

    if (!testFederation_.TryFindScriptableService(serviceName, nodeId, target))
    {
        TestSession::WriteWarning(TraceSource, "Failed to find service {0} one node {1}", serviceName, nodeId);

        return false;
    }

    auto from = begin(params) + 3;

    StringCollection commandParams(from, end(params));

    std::wstring result;
    ErrorCode error = target->ExecuteCommand(command, commandParams, result);

    if (!error.IsSuccess())
    {
        TestSession::WriteWarning(TraceSource, "Command {0} failed with error {1}", command, error);
        return false;
    }
    else if (result.empty())
    {
        TestSession::WriteInfo(TraceSource, "Command {0} succeeded.", command);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "Command {0} succeeded with result {1}.", command, result);
    }

    return true;
}

bool FabricTestDispatcher::ClientPut(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientPutCommand);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 key = static_cast<__int64>(Common::Int64_Parse(params[1]));
    wstring value = params[2];
    if(value == L"KeyDoesNotExist")
    {
        TestSession::WriteWarning(TraceSource, "'KeyDoesNotExist' is a reserved value and cannot be used");
        return false;
    }

    CommandLineParser parser(params, 3);

    vector<wstring> errorStrings;
    parser.GetVector(L"errors", errorStrings, L"");
    vector<ErrorCodeValue::Enum> expectedErrors;
    for (auto const & errorString : errorStrings)
    {
        auto errValue = ParseErrorCode(errorString);
        expectedErrors.push_back(errValue);
    }

    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    auto errValue = ParseErrorCode(errorString);
    expectedErrors.push_back(errValue);

    bool noVerify = parser.GetBool(L"noverify");

    int keyCount = 0;
    parser.TryGetInt(L"keycount", keyCount, 1);

    // If the specified 'valsize' is greater than the actual value.size(), append (valuesize - value.size()) chars to the end of value.
    // This can be used to construct big strings.
    int valSize = (int)value.size();
    parser.TryGetInt(L"valsize", valSize, (int)value.size());
    if (valSize > value.size()) {
        value.append(valSize - value.size(), L' ');
    }

    if (!noVerify)
    {
        FABRICSESSION.Expect("Put keys [{0}, {1}) for service {2} succeeded with expected error {3}", key, key+keyCount, serviceName, expectedErrors);
    }

    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root, client, key, keyCount, value, serviceName, expectedErrors, noVerify]()
    {
        bool quorumLost = false;
        bool done = false;
        int retryCount = 0;
        while(!done)
        {
            client->Put(key, keyCount, value, false /*useDefaultNamedClient*/, quorumLost, expectedErrors, noVerify);

            if(quorumLost && retryCount < FabricTestSessionConfig::GetConfig().StoreClientCommandQuorumLossRetryCount)
            {
                // Sleep for 3 seconds and update quorum lost list
                Sleep(3000);
                UpdateQuorumOrRebuildLostFailoverUnits();
                ++retryCount;
            }
            else
            {
                done = true;
            }
        }

        if(!quorumLost)
        {
            if (!noVerify)
            {
                FABRICSESSION.Validate("Put keys [{0}, {1}) for service {2} succeeded with expected error {3}", key, key+keyCount, serviceName, expectedErrors);
            }
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Put keys [{0}, {1}) for service {2} failed due to quorum loss.", key, key+keyCount, serviceName);
        }
    });

    return true;
}

bool FabricTestDispatcher::ClientGet(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientGetCommand);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 key = static_cast<__int64>(Common::Int64_Parse(params[1]));

    wstring expectedValue;
    bool checkExpected = params.size() >= 3;
    if (checkExpected)
    {
        expectedValue = params[2];
    }

    CommandLineParser parser(params, 2);

    int keyCount = 0;
    parser.TryGetInt(L"keycount", keyCount, 1);

    int valSize = (int)expectedValue.size();
    parser.TryGetInt(L"valsize", valSize, (int)expectedValue.size());

    FABRICSESSION.Expect("Get keys [{0}, {1}) for service {2} returned expected result", key, key+keyCount, serviceName);
    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root, client, key, keyCount, valSize, serviceName, checkExpected, expectedValue]()
    {
        vector<wstring> values;
        vector<bool> exists;
        bool quorumLost = false;

        bool done = false;
        int retryCount = 0;
        while(!done)
        {
            client->Get(key, keyCount, false /*useDefaultNamedClient*/, values, exists, quorumLost);

            if(quorumLost && retryCount < FabricTestSessionConfig::GetConfig().StoreClientCommandQuorumLossRetryCount)
            {
                // Sleep for 3 seconds and update quorum lost list
                Sleep(3000);
                UpdateQuorumOrRebuildLostFailoverUnits();
                ++retryCount;
            }
            else
            {
                done = true;
            }
        }

        // If 'valSize' specified, that means we appended chars to the actual value during 'Put'.
        // Here we need to strip the appended chars off in order to get the actual value.
        if (valSize > expectedValue.size())
        {
            for (auto & value : values)
            {
                value.resize(expectedValue.size());
            }
        }

        bool checkResult = false;
        if(checkExpected)
        {
            if (values.size() < keyCount || exists.size() < keyCount)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "Get keys [{0}, {1}) for service {2} has insufficient result count: values={3} exists={4}",
                    key,
                    key+keyCount,
                    serviceName,
                    values.size(),
                    exists.size());
            }
            else
            {
                if(expectedValue == L"KeyDoesNotExist")
                {
                    checkResult = true;
                    for (auto keyExists : exists)
                    {
                        if (keyExists)
                        {
                            checkResult = false;
                            break;
                        }
                    }
                }
                else
                {
                    checkResult = true;
                    for (auto keyExists : exists)
                    {
                        if (!keyExists)
                        {
                            checkResult = false;
                            break;
                        }
                    }

                    if (checkResult)
                    {
                        for (auto const & value : values)
                        {
                            if (value.compare(expectedValue) != 0)
                            {
                                checkResult = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if(!quorumLost && checkResult)
        {
            FABRICSESSION.Validate("Get keys [{0}, {1}) for service {2} returned expected result", key, key+keyCount, serviceName);
        }
        else
        {
            // vector<bool> can't be traced on Linux
            wstring existsString;
            StringWriter w(existsString);
            for (auto keyExists : exists)
            {
                w.Write(keyExists ? "T" : "F");
            }
            
            TestSession::WriteInfo(
                TraceSource,
                "Get keys [{0}, {1}) for service {2} failed. KeyExists ({3}), QuorumLost {4}, ExpectedValue {5}, ActualValues {6}",
                key,
                key+keyCount,
                serviceName,
                existsString,
                quorumLost,
                checkExpected ? expectedValue : L"NA",
                values);
        }

        TestSession::WriteInfo(TraceSource, "Values are {0} for keys [{1}, {2}):{3}.", values, key, key+keyCount, serviceName);
    });

    return true;
}

bool FabricTestDispatcher::ClientGetAll(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientGetAllCommand);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    wstring values = params[1];
    vector<wstring> elems;
    size_t begin = values.find_first_not_of(L"(");
    size_t end = values.find_first_of(L")", begin);
    StringUtility::Split<wstring>(values.substr(begin, end-begin), elems, FabricTestDispatcher::ItemDelimiter);

    vector<pair<__int64, wstring>> expectedKVpairs;
    for (auto & s : elems)
    {
        size_t pos = s.find_first_of(L":");
        wstring key = s.substr(0, pos);
        wstring val = s.substr(pos + 1);
        expectedKVpairs.emplace_back(static_cast<__int64>(Common::Int64_Parse(key.c_str())), val);
    }

    bool ordered = false;
    if (params.size() > 2 && params[2] == L"ordered")
    {
        ordered = true;
    }

    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root, client, serviceName, expectedKVpairs]()
    {
        vector<pair<__int64, wstring>> kvpairs;
        bool quorumLost = false;

        bool done = false;
        int retryCount = 0;
        while (!done)
        {
            client->GetAll(kvpairs, quorumLost);

            if (quorumLost && retryCount < FabricTestSessionConfig::GetConfig().StoreClientCommandQuorumLossRetryCount)
            {
                // Sleep for 3 seconds and update quorum lost list
                Sleep(3000);
                UpdateQuorumOrRebuildLostFailoverUnits();
                ++retryCount;
            }
            else
            {
                done = true;
            }
        }

        if (kvpairs.size() != expectedKVpairs.size())
        {
            TestSession::FailTest(
                "GetAll for service {0} has incorrect result count: expectedKeyValPairNum={1} existsNum={2}",
                serviceName,
                expectedKVpairs.size(),
                kvpairs.size());
        }

        for (int i = 0; i < kvpairs.size(); ++i)
        {
            if (kvpairs[i].first != expectedKVpairs[i].first ||
                kvpairs[i].second != expectedKVpairs[i].second)
            {
                TestSession::FailTest(
                    "GetAll for service {0} failed. QuorumLost {1}, ExpectedKey {2} ExpectedValue {3}, ActualKey {4} ActualValue {5}",
                    serviceName,
                    quorumLost,
                    expectedKVpairs[i].first,
                    expectedKVpairs[i].second,
                    kvpairs[i].first,
                    kvpairs[i].second);
            }
        }
    });

    return true;
}

bool FabricTestDispatcher::ClientGetKeys(StringCollection const & params)
{
    // usage: clientgetkeys servicename firstkey lastkey (key1:value1,key2:value2,...)
    // If value1 is empty string, only key1 will be compared. Same for the rest of the kv pairs.
    if (params.size() < 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientGetKeysCommand);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 first = static_cast<__int64>(Common::Int64_Parse(params[1]));
    __int64 last = static_cast<__int64>(Common::Int64_Parse(params[2]));

    wstring s = params[3];
    vector<wstring> elems;
    size_t begin = s.find_first_not_of(L"(");
    size_t end = s.find_first_of(L")", begin);
    StringUtility::Split<wstring>(s.substr(begin, end - begin), elems, FabricTestDispatcher::ItemDelimiter);

    vector<__int64> expectedKeys;
    for (auto & e : elems)
    {
        expectedKeys.push_back(static_cast<__int64>(Common::Int64_Parse(e)));
    }

    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root, client, serviceName, first, last, expectedKeys]()
    {
        vector<__int64> keys;
        bool quorumLost = false;

        bool done = false;
        int retryCount = 0;
        while (!done)
        {
            client->GetKeys(first, last, keys, quorumLost);

            if (quorumLost && retryCount < FabricTestSessionConfig::GetConfig().StoreClientCommandQuorumLossRetryCount)
            {
                // Sleep for 3 seconds and update quorum lost list
                Sleep(3000);
                UpdateQuorumOrRebuildLostFailoverUnits();
                ++retryCount;
            }
            else
            {
                done = true;
            }
        }

        if (keys.size() != expectedKeys.size())
        {
            TestSession::FailTest(
                "GetKeys for service {0} has incorrect result count: expectedKeysNum={1} existsNum={2}",
                serviceName,
                expectedKeys.size(),
                keys.size());
        }

        for (int i = 0; i < keys.size(); ++i)
        {
            if (keys[i] != expectedKeys[i])
            {
                TestSession::FailTest(
                    "GetKeys for service {0} failed. QuorumLost {1}, ExpectedKey {2}, ActualKey {3}",
                    serviceName,
                    quorumLost,
                    expectedKeys[i],
                    keys[i]);
            }
        }
    });

    return true;
}

bool FabricTestDispatcher::ClientDelete(StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientDeleteCommand);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 key = static_cast<__int64>(Common::Int64_Parse(params[1]));

    CommandLineParser parser(params, 2);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum expectedError = ParseErrorCode(errorString);
    bool noVerify = parser.GetBool(L"noverify");

    int keyCount = 0;
    parser.TryGetInt(L"keycount", keyCount, 1);

    if (!noVerify)
    {
        FABRICSESSION.Expect("Delete keys [{0}, {1}) for service {2} succeeded with expected error {3}", key, key+keyCount, serviceName, expectedError);
    }

    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root, client, key, keyCount, serviceName, expectedError, noVerify]()
    {
        bool quorumLost = false;
        bool done = false;
        int retryCount = 0;
        while(!done)
        {
            client->Delete(key, keyCount, false /*useDefaultNamedClient*/, quorumLost, expectedError, noVerify);

            if(quorumLost && retryCount < FabricTestSessionConfig::GetConfig().StoreClientCommandQuorumLossRetryCount)
            {
                // Sleep for 3 seconds and update quorum lost list
                Sleep(3000);
                UpdateQuorumOrRebuildLostFailoverUnits();
                ++retryCount;
            }
            else
            {
                done = true;
            }
        }

        if(!quorumLost)
        {
            if (!noVerify)
            {
                FABRICSESSION.Validate("Delete keys [{0}, {1}) for service {2} succeeded with expected error {3}", key, key+keyCount, serviceName, expectedError);
            }
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Delete keys [{0}, {1}) for service {2} failed due to quorum loss.", key, key+keyCount, serviceName);
        }
    });

    return true;
}

bool FabricTestDispatcher::StartClient(StringCollection const & params)
{
    if (params.size() != 1 && params.size() != 3 && params.size() != 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::StartClientCommand);
        return false;
    }

    bool startThreads = false;
    {
        AcquireWriteLock lock(testClientsMapLock_);

        if(params.size() == 1)
        {
            if(clientsStopped_)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Incorrect number of arguments provided to startclient when clients are not active.");
                return false;
            }

            wstring serviceName = params[0];
            TestSession::FailTestIfNot(testClients_.find(serviceName) == testClients_.end(), "Client for service {0} already exists", serviceName);
            testClients_[serviceName] = make_shared<TestStoreClient>(serviceName);
            testClients_[serviceName]->StartClient();
            if(testClients_.size() == 1)
            {
                startThreads = true;
            }
        }
        else
        {
            if(!clientsStopped_)
            {
                TestSession::WriteError(
                    TraceSource,
                    "Incorrect number of arguments provided to startclient when clients already active.");
                return false;
            }

            ULONG threads = static_cast<ULONG>(Common::Int64_Parse(params[0]));
            double putRatio = Common::Double_Parse(params[1]);
            int64 clientOperationInterval = Common::Int64_Parse(params[2]);
            if(params.size() == 3)
            {
                vector<wstring> services;
                ServiceMap.GetAdhocStatefulServices(services);
                for (auto it = services.begin() ; it != services.end(); it++ )
                {
                    wstring const& serviceName = (*it);
                    if (serviceName.compare(ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName) != 0 &&
                        serviceName.compare(ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName) != 0 &&
                        serviceName.compare(ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName) != 0)
                    {
                        testClients_[(*it)] = make_shared<TestStoreClient>(serviceName);
                        testClients_[(*it)]->StartClient();
                    }
                }
            }
            else
            {
                wstring serviceName = params[3];
                TestSession::FailTestIfNot(testClients_.find(serviceName) == testClients_.end(), "Client for service {0} already exists", serviceName);
                testClients_[serviceName] = make_shared<TestStoreClient>(serviceName);
                testClients_[serviceName]->StartClient();
            }

            clientThreads_ = threads;
            putRatio_ = putRatio;
            maxClientOperationInterval_ = clientOperationInterval;
            clientsStopped_ = false;

            if(testClients_.size() > 0)
            {
                startThreads = true;
            }
        }
    }

    if(startThreads)
    {
        for(ULONG i = 0; i < clientThreads_; i++)
        {
            AcquireExclusiveLock lock(lastClientOperationMapLock_);
            lastClientOperationMap_[i] = DateTime::Now();
            auto root = FABRICSESSION.CreateComponentRoot();
            Threadpool::Post([this, root, i]()
            {
                DoWork(i);
            });
        }

        VerifyClientIsActive();
    }

    return true;
}

void FabricTestDispatcher::VerifyClientIsActive()
{
    if(!clientsStopped_)
    {
        AcquireExclusiveLock lock(lastClientOperationMapLock_);
        for(ULONG i = 0; i < clientThreads_; i++)
        {
            TestSession::FailTestIf(FabricTestSessionConfig::GetConfig().StoreClientTimeout < (DateTime::Now() - lastClientOperationMap_[i]),
                "Client has thread {0} inactive for more than {1} seconds",
                i,
                FabricTestSessionConfig::GetConfig().StoreClientTimeout.TotalSeconds());
        }

        auto root = FABRICSESSION.CreateComponentRoot();
        Threadpool::Post([this, root]()
        {
            VerifyClientIsActive();
        }, TimeSpan::FromMilliseconds(ClientActivityCheckIntervalInMilliseconds));
    }
}

void FabricTestDispatcher::DoWork(ULONG threadId)
{
    TestSession::WriteNoise(TraceSource, "Client thread {0} invoked", threadId);
    Random random;
    shared_ptr<TestStoreClient> client;
    {
        AcquireReadLock lock(testClientsMapLock_);
        if(!clientsStopped_)
        {
            auto iterator = testClients_.begin();
            advance(iterator, random.Next(0, static_cast<int>(testClients_.size())));
            client = iterator->second;
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Client thread {0} done", threadId);
            return;
        }
    }

    __int64 key = static_cast<__int64>(random.Next(static_cast<int>(FabricTestSessionConfig::GetConfig().ServiceDescriptorLowRange), static_cast<int>(FabricTestSessionConfig::GetConfig().ServiceDescriptorHighRange) + 1));
    bool doPut = random.NextDouble() < putRatio_;
    client->DoWork(threadId, key, doPut);

    {
        AcquireExclusiveLock lock(lastClientOperationMapLock_);
        lastClientOperationMap_[threadId] = DateTime::Now();
        TestSession::WriteNoise(TraceSource, "Client thread {0} reported activity at {1}", threadId, lastClientOperationMap_[threadId]);
    }

    auto root = FABRICSESSION.CreateComponentRoot();
    Threadpool::Post([this, root, threadId]()
    {
        DoWork(threadId);
    }, TimeSpan::FromMilliseconds(random.NextDouble() * maxClientOperationInterval_));

}

bool FabricTestDispatcher::StopClient(StringCollection const & params)
{
    if (params.size() != 0 && params.size() != 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::StopClientCommand);
        return false;
    }

    AcquireWriteLock lock(testClientsMapLock_);
    if(params.size() == 0 || testClients_.size() == 1)
    {
        clientsStopped_ = true;
        checkpointTruncationTimestampValidator_->Stop();

        map<wstring, shared_ptr<TestStoreClient>> testClients(testClients_.begin(), testClients_.end());
        testClients_.clear();
        for (auto it = testClients.begin() ; it != testClients.end(); it++ )
        {
            it->second->WaitForActiveThreadsToFinish();
        }
    }
    else if (params.size() == 1)
    {
        wstring serviceName = params[0];
        TestSession::FailTestIf(testClients_.find(serviceName) == testClients_.end(), "Client for service {0} does not exist", serviceName);
        shared_ptr<TestStoreClient> testClient = testClients_[serviceName];
        testClients_.erase(serviceName);
        testClient->WaitForActiveThreadsToFinish();
    }

    return true;
}

bool FabricTestDispatcher::ClientBackup(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientBackup);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 key = static_cast<__int64>(Common::Int64_Parse(params[1]));

    wstring backupDir = params[2];

    CommandLineParser parser(params, 3);
    wstring errorString;
    wstring backupOptionString;
    wstring postBackupReturnString;

    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum expectedError = ParseErrorCode(errorString);

    parser.TryGetString(L"backupOption", backupOptionString, L"Full");

    parser.TryGetString(L"postBackupReturn", postBackupReturnString, L"true");

    bool postBackupReturn = IsTrue(postBackupReturnString);

    StoreBackupOption::Enum backupOption;

    if (StringUtility::AreEqualCaseInsensitive(backupOptionString, L"Full")) { backupOption = StoreBackupOption::Full; }
    else if (StringUtility::AreEqualCaseInsensitive(backupOptionString, L"Incremental")) { backupOption = StoreBackupOption::Incremental; }
    else if (StringUtility::AreEqualCaseInsensitive(backupOptionString, L"TruncateLogsOnly"))
    {
        backupOption = StoreBackupOption::TruncateLogsOnly;
        backupDir = L""; // Backup requires an empty parameter if TruncateLogsOnly is used.
    }
    else
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientBackup);
        return false;
    }

    if (backupDir.compare(L"CurrentDirectory") == 0)
    {
        backupDir = Common::Directory::GetCurrentDirectoryW();
    }

    client->Backup(key, backupDir, backupOption, postBackupReturn, expectedError);

    return true;
}

bool FabricTestDispatcher::ClientRestore(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientRestore);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 key = static_cast<__int64>(Common::Int64_Parse(params[1]));

    wstring backupDir = params[2];

    CommandLineParser parser(params, 3);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum expectedError = ParseErrorCode(errorString);

    client->Restore(key, backupDir, expectedError);

    client->Reset();

    return true;
}

bool FabricTestDispatcher::ClientCompression(StringCollection const & params)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ClientCompressionTest);
        return false;
    }

    wstring serviceName = params[0];
    TestStoreClientSPtr client = GetTestClient(serviceName);

    __int64 key = static_cast<__int64>(Common::Int64_Parse(params[1]));

    vector<wstring> testArgs;
    StringUtility::Split<wstring>(params[2], testArgs, L",");

    CommandLineParser parser(params, 3);
    wstring errorString;
    parser.TryGetString(L"error", errorString, L"Success");
    ErrorCodeValue::Enum expectedError = ParseErrorCode(errorString);

    client->CompressionTest(key, testArgs, expectedError);

    client->Reset();

    return true;
}

bool FabricTestDispatcher::SetBackwardsCompatibleClients(StringCollection const & params)
{
    bool backwardsCompatible = true;

    if (params.size() >= 1)
    {
        backwardsCompatible = IsTrue(params[0]);
    }

    UseBackwardsCompatibleClients = backwardsCompatible;

    return true;
}

bool FabricTestDispatcher::StartTestFabricClient(StringCollection const & params)
{
    if (params.size() != 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::StartTestFabricClientCommand);
        return false;
    }

    ULONG threads = static_cast<ULONG>(Common::Int64_Parse(params[0]));
    ULONG nameCount = static_cast<ULONG>(Common::Int64_Parse(params[1]));
    ULONG propertiesPerName = static_cast<ULONG>(Common::Int64_Parse(params[2]));
    int64 clientOperationInterval = Common::Int64_Parse(params[3]);
    FabricClient.StartTestFabricClient(threads, nameCount, propertiesPerName, clientOperationInterval);
    return true;
}

bool FabricTestDispatcher::VerifyLoadReport(Common::StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyLoadReportCommand);
        return false;
    }

    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);
    TestSession::FailTestIf(!failoverUnit, "FailoverUnit {0} not found", params[0]);
    int numberOfUpdates = Common::Int32_Parse(params[1]);


    std::wstring serviceName = params[0];
    TestSession::FailTestIfNot(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabled, "VerifyLoadReport called when DummyPLB not enabled");

    auto fm = GetFM();

    if(!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready");
        return false;
    }

    auto plb = fm->PLBTestHelper;

    TestSession::FailTestIf(!plb, "PLB not valid");

    TestSession::FailTestIfNot(plb->CheckLoadReport(serviceName, failoverUnit->Id.Guid, numberOfUpdates), "VerifyLoadReport failed");

    return true;
}

bool FabricTestDispatcher::VerifyLoadValue(Common::StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);
    TestSession::FailTestIf(!failoverUnit, "FailoverUnit {0} not found", params[0]);

    std::wstring serviceName = params[0];
    wstring metricName = params[1];

    Reliability::LoadBalancingComponent::ReplicaRole::Enum role =
        (0 == StringUtility::CompareCaseInsensitive(params[2].c_str(), L"primary")) ?
        Reliability::LoadBalancingComponent::ReplicaRole::Primary :
    Reliability::LoadBalancingComponent::ReplicaRole::Secondary;

    if ((role == Reliability::LoadBalancingComponent::ReplicaRole::Secondary && params.size() != 5) ||
        (role == Reliability::LoadBalancingComponent::ReplicaRole::Primary && params.size() != 4))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyLoadValueCommand);
        return false;
    }

    uint value = Common::Int32_Parse(params[3]);
    TestSession::FailTestIfNot(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabled, "VerifyLoadValue called when DummyPLB not enabled");
    auto fm = GetFM();

    if(!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready");
        return false;
    }

    auto plb = fm->PLBTestHelper;

    TestSession::FailTestIf(!plb, "PLB not valid");

    NodeId nodeId;
    if (role == Reliability::LoadBalancingComponent::ReplicaRole::Secondary)
    {
        nodeId = ParseNodeId(params[4]);
    }

    TestSession::FailTestIfNot(plb->CheckLoadValue(serviceName, failoverUnit->Id.Guid, metricName, role, value, nodeId), "VerifyLoadValue failed");

    return true;
}

bool FabricTestDispatcher::VerifyNodeLoad(Common::StringCollection const & params)
{
    if (params.size() < 3 || params.size() > 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyNodeLoadCommand);
        return false;
    }

    wstring nodeName = params[0];
    wstring metricName = params[1];
    int64 nodeLoad = Common::Int64_Parse(params[2]);
    double doubleLoad = -1.0;
    if (params.size() == 4)
    {
        doubleLoad = Common::Double_Parse(params[3]);
    }

    FabricClient.VerifyNodeLoad(nodeName, metricName, nodeLoad, doubleLoad);
    return true;
}

bool FabricTestDispatcher::VerifyPartitionLoad(Common::StringCollection const & params)
{
    if (params.size() != 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyPartitionLoadCommand);
        return false;
    }

    FABRIC_PARTITION_ID partitionId = Guid(params[0]).AsGUID();
    wstring metricName = params[1];
    int64 expectedPrimaryLoad = Common::Int64_Parse(params[2]);
    int64 expectedSecondaryLoad = Common::Int64_Parse(params[3]);

    FabricClient.VerifyPartitionLoad(partitionId, metricName, expectedPrimaryLoad, expectedSecondaryLoad);
    return true;
}

bool FabricTestDispatcher::VerifyClusterLoad(Common::StringCollection const & params)
{
    if (params.size() != 4 && params.size() != 5 && params.size() != 8)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyClusterLoadCommand);
        return false;
    }

    wstring metricName = params[0];
    int64 clusterLoad = Common::Int64_Parse(params[1]);
    int64 maxNodeLoad = Common::Int64_Parse(params[2]);
    int64 minNodeLoad = Common::Int64_Parse(params[3]);

    double deviation = -1;
    if (params.size() >= 5)
    {
        deviation = Common::Double_Parse(params[4]);
    }

    double expectedDoubleLoad = -1;
    double expectedDoubleMaxNodeLoad = -1;
    double expectedDoubleMinNodeLoad = -1;

    if (params.size() == 8)
    {
        expectedDoubleLoad = Common::Double_Parse(params[5]);
        expectedDoubleMaxNodeLoad = Common::Double_Parse(params[6]);
        expectedDoubleMinNodeLoad = Common::Double_Parse(params[7]);
    }

    FabricClient.VerifyClusterLoad(metricName, clusterLoad, maxNodeLoad, minNodeLoad, deviation, expectedDoubleLoad, expectedDoubleMaxNodeLoad, expectedDoubleMinNodeLoad);
    return true;
}

bool FabricTestDispatcher::VerifyUnplacedReason(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyUnplacedReasonCommand);
        return false;
    }

    wstring serviceName = params[0];
    wstring reason;
    for (int i = 1; i < params.size() - 1; ++i)
    {
        reason +=params[i] + L" ";
    }

    reason += params[params.size() - 1];

    FabricClient.VerifyUnplacedReason(serviceName, reason);
    return true;
}

bool FabricTestDispatcher::VerifyApplicationLoad(Common::StringCollection const & params)
{
    bool validationOK = false;

    wstring applicationName = params[0];

    CommandLineParser parser(params, 1);

    TimeoutHelper timeoutHelper(TimeSpan::FromSeconds(60.0));

    while (!timeoutHelper.IsExpired && !validationOK)
    {
        validationOK = true;

        ServiceModel::ApplicationLoadInformationQueryResult queryResult;

        FabricClient.GetApplicationLoadInformation(applicationName, queryResult);

        if (applicationName != queryResult.ApplicationName)
        {
            TestSession::WriteError(
                TraceSource,
                "Provided application name ({0}) does not match the query ({1}).",
                applicationName,
                queryResult.ApplicationName);
            return false;
        }

        int64 maximumNodes;
        if (parser.TryGetInt64(L"maximumNodes", maximumNodes))
        {
            if (maximumNodes != queryResult.MaximumNodes)
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Provided maximumNodes ({0}) does not match the query ({1}).",
                    maximumNodes,
                    queryResult.MaximumNodes);
                validationOK = false;
            }
        }

        int64 minimumNodes;
        if (parser.TryGetInt64(L"minimumNodes", minimumNodes))
        {
            if (minimumNodes != queryResult.MinimumNodes)
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Provided minimumNodes ({0}) does not match the query ({1}).",
                    minimumNodes,
                    queryResult.MinimumNodes);
                validationOK = false;
            }
        }

        bool verifyNodeCount = parser.GetBool(L"verifynodecount");

        if (verifyNodeCount)
        {
            if (queryResult.MinimumNodes > queryResult.NodeCount
                || queryResult.MaximumNodes < queryResult.NodeCount)
            {
                // Node count should be between min nodes and max nodes
                TestSession::WriteWarning(
                    TraceSource,
                    "Node count ({0}) is not in the interval ({1},{2}).",
                    queryResult.NodeCount,
                    queryResult.MinimumNodes,
                    queryResult.MaximumNodes);
                validationOK = false;
            }
        }

        int64 nodeCount;
        if (parser.TryGetInt64(L"nodeCount", nodeCount))
        {
            if (nodeCount != queryResult.NodeCount)
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Provided nodeCount ({0}) does not match the query ({1}).",
                    nodeCount,
                    queryResult.NodeCount);
                validationOK = false;
            }
        }

        Common::StringCollection metricCollection;
        if (parser.GetVector(L"loads", metricCollection))
        {
            if ((metricCollection.size() % 4) != 0)
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Invalid specification of application metrics.");
                return false;
            }

            uint metricCount = static_cast<uint>(metricCollection.size()) / 4;

            for (uint i = 0; i < metricCount; i++)
            {
                wstring metricName = metricCollection[4 * i];
                int64 reservedCapacity = Common::Int64_Parse(metricCollection[4 * i + 1]);
                int64 totalCapacity = Common::Int64_Parse(metricCollection[4 * i + 2]);
                int64 load = Common::Int64_Parse(metricCollection[4 * i + 3]);
                bool found = false;
                for (auto metricResult : queryResult.ApplicationLoadMetricInformation)
                {
                    if (metricResult.Name != metricName)
                    {
                        continue;
                    }

                    found = true;

                    if (reservedCapacity != metricResult.ReservationCapacity)
                    {
                        TestSession::WriteWarning(
                            TraceSource,
                            "Provided reservedCapacity ({0}) does not match the query ({1}) for metric {2}.",
                            reservedCapacity,
                            metricResult.ReservationCapacity,
                            metricName);
                        validationOK = false;
                    }

                    if (totalCapacity != metricResult.ApplicationCapacity)
                    {
                        TestSession::WriteWarning(
                            TraceSource,
                            "Provided totalCapacity ({0}) does not match the query ({1}) for metric {2}.",
                            totalCapacity,
                            metricResult.ApplicationCapacity,
                            metricName);
                        validationOK = false;
                    }

                    if (load != metricResult.ApplicationLoad)
                    {
                        TestSession::WriteWarning(
                            TraceSource,
                            "Provided load value ({0}) does not match the query ({1}) for metric {2}.",
                            load,
                            metricResult.ApplicationLoad,
                            metricName);
                        validationOK = false;
                    }
                }

                if (!found)
                {
                    TestSession::WriteError(
                        TraceSource,
                        "Metric {0} not found in application load query result.",
                        metricName);
                    return false;
                }
            }
        }

        Sleep(3000);
    }

    TestSession::FailTestIfNot(validationOK, "VerifyApplicationLoad failed");

    return true;
}

bool FabricTestDispatcher::UpdateNodeImages(Common::StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::UpdateNodeImagesCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    FabricTestNodeSPtr testNode = testFederation_.GetNode(nodeId);

    if (testNode == nullptr)
    {
        TestSession::WriteWarning(
            TraceSource,
            "Cannot find node with ID {0}.",
            nodeId);
        return false;
    }

    shared_ptr<FabricNodeWrapper> nodeWrapper;
    if (!testNode->TryGetFabricNodeWrapper(nodeWrapper))
    {
        TestSession::WriteWarning(
            TraceSource,
            "Cannot get node wrapper for ID {0}.",
            nodeId);
        return false;
    }

    wstring nodeIdString = nodeWrapper->GetHostingSubsystem().NodeId;

    CommandLineParser parser(params, 1);
    wstring availableImagesString;
    parser.TryGetString(L"images", availableImagesString, L"");
    Common::StringCollection imagesCollection;
    Common::StringUtility::Split<std::wstring>(availableImagesString, imagesCollection, L",");

    vector<wstring> availableImagesOnNode;
    for (size_t index = 0; index < imagesCollection.size(); ++index)
    {
        availableImagesOnNode.push_back(imagesCollection[index]);
    }

    auto fm = GetFM();

    if (!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready");
        return false;
    }

    auto plb = fm->PLBTestHelper;

    TestSession::FailTestIf(!plb, "PLB not valid");

    plb->UpdateAvailableImagesPerNode(nodeIdString, availableImagesOnNode);

    return true;
}

bool FabricTestDispatcher::VerifyResourceOnNode(Common::StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyResourceOnNodeCommand);
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    FabricTestNodeSPtr testNode = testFederation_.GetNode(nodeId);

    if (testNode == nullptr)
    {
        TestSession::WriteWarning(
            TraceSource,
            "Cannot find node with ID {0}.",
            nodeId);
        return false;
    }

    shared_ptr<FabricNodeWrapper> nodeWrapper;
    if (!testNode->TryGetFabricNodeWrapper(nodeWrapper))
    {
        TestSession::WriteWarning(
            TraceSource,
            "Cannot get node wrapper for ID {0}.",
            nodeId);
        return false;
    }

    wstring resourceName = params[1];
    double expectedValue = Common::Double_Parse(params[2]);

    TimeoutHelper timeoutHelper(TimeSpan::FromSeconds(60.0));

    bool validationOK = false;

    while (!timeoutHelper.IsExpired && !validationOK)
    {
        auto realValue = nodeWrapper->GetHostingSubsystem().LocalResourceManagerObj->GetResourceUsage(resourceName);
        bool realValueExpected = realValue < 0.0 || fabs(realValue - expectedValue) < 0.001;
        if (realValueExpected)
        {
            validationOK = true;
        }
        else
        {
            TestSession::WriteWarning(
                TraceSource,
                "verifyresourceonnode {0} {1} {2} - retreived value {3}, retrying in 5 seconds...",
                params[0],
                params[1],
                params[2],
                realValue);
            Sleep(5000);
        }
    }

    TestSession::FailTestIfNot(validationOK, "VerifyResourceOnNode failed");

    return validationOK;
}

bool FabricTestDispatcher::VerifyLimitsEnforced(Common::StringCollection const & params)
{
    std::vector<double> expectedCpuLimitCP;
    bool checkCpuLimit = false;
    std::vector<uint64> expectedMemoryLimitCP;
    const int MaxNumberOfRetries = 3;

    for (auto & param : params)
    {
        vector<wstring> values;
        StringUtility::Split<wstring>(param, values, L"=");

        vector<wstring> expectedValues;

        if (values.size() > 1)
        {
            StringUtility::Split<wstring>(values[1], expectedValues, L",");
        }

        for (auto & val : expectedValues)
        {
            if (StringUtility::AreEqualCaseInsensitive(values[0], L"CpCpu"))
            {
                checkCpuLimit = true;
                double cpuLoad;

                if (val.back() == L'w')
                {
                    //for linux we ignore these values as they are only applied on Windows
#if defined(PLATFORM_UNIX)
                    continue;
#else
                    val = val.substr(0, val.size() - 1);
#endif
                }
                TryParseDouble(val, cpuLoad);
                expectedCpuLimitCP.push_back(cpuLoad);
            }
            else if (StringUtility::AreEqualCaseInsensitive(values[0], L"CpMemory"))
            {
                uint64 memoryLoad;
                if (val.back() == L'w')
                {
                    //for linux we ignore these values as they are only applied on Windows
#if defined(PLATFORM_UNIX)
                    continue;
#else
                    val = val.substr(0, val.size() - 1);
#endif
                }
                TryParseUInt64(val, memoryLoad);
                expectedMemoryLimitCP.push_back(memoryLoad);
            }

        }
    }
    std::vector<double> fetchedCpuLimitCP;
    std::vector<uint64> fetchedMemoryLimitCP;

    auto & processActivationManager = fabricActivationManager_->ProcessActivationManagerObj;
    bool isEnforced = false;

    for (int i = 0; i < MaxNumberOfRetries; ++i)
    {
        if (processActivationManager->Test_VerifyLimitsEnforced(fetchedCpuLimitCP, fetchedMemoryLimitCP))
        {
            isEnforced = true;
            break;
        }
    }

    TestSession::FailTestIfNot(isEnforced, "Limits are not enforced properly");

    std::sort(expectedCpuLimitCP.begin(), expectedCpuLimitCP.end());
    std::sort(fetchedCpuLimitCP.begin(), fetchedCpuLimitCP.end());
    std::sort(expectedMemoryLimitCP.begin(), expectedMemoryLimitCP.end());
    std::sort(fetchedMemoryLimitCP.begin(), fetchedMemoryLimitCP.end());

    auto checkDoubleSame = [&](std::vector<double> const & expected, std::vector<double> const & real)
    {
        bool isOk = true;
        if (expected.size() != real.size())
        {
            isOk = false;
        }
        else for (int i = 0; i < expected.size(); ++i)
        {
            if (fabs(real[i] - expected[i]) > 0.01)
            {
                isOk = false;
                break;
            }
        }
        if (!isOk)
        {
            std::string errorMessage = "Cpu limits do not match size expected ";
            errorMessage += "Expected:{";
            for (int i = 0; i < expected.size(); ++i)
            {
                errorMessage += formatString("{0},", expected[i]);
            }
            errorMessage += "}Actual:{";
            for (int i = 0; i < real.size(); ++i)
            {
                errorMessage += formatString("{0},", real[i]);
            }

            TestSession::FailTest("Limits not enforced : {0}", errorMessage);
        }
        return isOk;
    };
    auto checkIntSame = [&](std::vector<uint64> const & expected, std::vector<uint64> const & real)
    {
        bool isOk = true;
        if (expected.size() != real.size())
        {
            isOk = false;
        }
        else for (int i = 0; i < expected.size(); ++i)
        {
            if (real[i] != expected[i])
            {
                isOk = false;
                break;
            }
        }
        if (!isOk)
        {
            std::string errorMessage = "Memory limits do not match size expected ";
            errorMessage += "Expected:{";
            for (int i = 0; i < expected.size(); ++i)
            {
                errorMessage += formatString("{0},", expected[i]);
            }
            errorMessage += "}Actual:{";
            for (int i = 0; i < real.size(); ++i)
            {
                errorMessage += formatString("{0},", real[i]);
            }

            TestSession::FailTest("Limits not enforced : {0}", errorMessage);
        }
        return isOk;
    };

    if (checkCpuLimit)
    {
        isEnforced = isEnforced && checkDoubleSame(expectedCpuLimitCP, fetchedCpuLimitCP);
    }
    isEnforced = isEnforced && checkIntSame(expectedMemoryLimitCP, fetchedMemoryLimitCP);

    return isEnforced;
}

bool FabricTestDispatcher::VerifyPLBAndLRMSync(Common::StringCollection const & params)
{
    // For each node:
    //  - Get load information from LRM
    //      - Sum all values for resources from deployed service packages.
    //      - Check that they are in sync
    //  - Get load information from PLB.
    //      - Check that it matches what LRM sees.
    if (params.size() != 0)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyResourceOnNodeCommand);
        return false;
    }

    TimeSpan timeout = FabricTestSessionConfig::GetConfig().VerifyTimeout;
    TimeoutHelper timeoutHelper(timeout);

    bool validationOK = false;
    while (!validationOK && !timeoutHelper.IsExpired)
    {
        validationOK = true;
        auto allNodes = testFederation_.GetNodes();
        for (FabricTestNodeSPtr testNode : allNodes)
        {
            shared_ptr<FabricNodeWrapper> nodeWrapper;
            if (!testNode->TryGetFabricNodeWrapper(nodeWrapper))
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "Cannot get node wrapper for ID {0}.",
                    testNode->Id);
                validationOK = false;
                continue;
            }

            double deployedCpuUsed;
            double deployedMemoryUsed;
            testNode->GetResourceUsageFromDeployedPackages(deployedCpuUsed, deployedMemoryUsed);

            auto cpuCoresUsedLRM = nodeWrapper->GetHostingSubsystem().LocalResourceManagerObj->GetResourceUsage(*ServiceModel::Constants::SystemMetricNameCpuCores);
            auto memoryUsedLRM = nodeWrapper->GetHostingSubsystem().LocalResourceManagerObj->GetResourceUsage(*ServiceModel::Constants::SystemMetricNameMemoryInMB);

            if (fabs(cpuCoresUsedLRM - deployedCpuUsed) > 0.001)
            {
                validationOK = false;
                TestSession::WriteWarning(
                    TraceSource,
                    "verifyplbandlrmsync: CPU for node {0} - LRM usage {1}, deployed sum: {2}",
                    testNode->Id,
                    cpuCoresUsedLRM,
                    deployedCpuUsed);
                continue;
            }

            if (fabs(memoryUsedLRM - deployedMemoryUsed) > 0.001)
            {
                validationOK = false;
                TestSession::WriteWarning(
                    TraceSource,
                    "verifyplbandlrmsync: Memory for node {0} - LRM usage {1}, deployed sum: {2}",
                    testNode->Id,
                    memoryUsedLRM,
                    deployedMemoryUsed);
                continue;
            }

            wstring nodeName = L"nodeid:" + testNode->Id.ToString();

            double cpuCoresUsedPLB;
            double memoryUsedPLB;
            fabricClient_->GetNodeLoadForResources(nodeName, cpuCoresUsedPLB, memoryUsedPLB);

            // Check CPU cores (PLB-LRM)
            bool cpuCoresValueExpected = fabs(cpuCoresUsedLRM - cpuCoresUsedPLB) < 0.001;
            if (!cpuCoresValueExpected)
            {
                validationOK = false;
                TestSession::WriteWarning(
                    TraceSource,
                    "verifyplbandlrmsync: CPU for node {0}({1}) - LRM usage {2}, PLB usage {3}",
                    testNode->Id,
                    nodeName,
                    cpuCoresUsedLRM,
                    cpuCoresUsedPLB);
                continue;
            }

            // Check Memory used
            bool memoryValueExpected = fabs(memoryUsedLRM - memoryUsedPLB) < 0.001;
            if (!memoryValueExpected)
            {
                validationOK = false;
                TestSession::WriteWarning(
                    TraceSource,
                    "verifyplbandlrmsync: Memory for node {0}({1}) - LRM usage {2}, PLB usage {3}",
                    testNode->Id,
                    nodeName,
                    memoryUsedLRM,
                    memoryValueExpected);
                continue;
            }
        }
    }

    TestSession::FailTestIfNot(validationOK, "verifyplbandlrmsync failed");

    return validationOK;
}

bool FabricTestDispatcher::VerifyContainerPods(Common::StringCollection const & params)
{
#if defined(PLATFORM_UNIX)
    const int MaxNumberOfRetries = 3;
    vector<int> containerGroups;
    for (auto & param : params)
    {
        int64 val;
        if (!TryParseInt64(param, val))
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::VerifyContainerPodsCommand);
            return false;
        }
        containerGroups.push_back((int)val);
    }
    auto & processActivationManager = fabricActivationManager_->ProcessActivationManagerObj;

    bool isSuccess = false;
    for (int i = 0; i < MaxNumberOfRetries; ++i)
    {
        if (processActivationManager->Test_VerifyPods(containerGroups))
        {
            isSuccess = true;
            break;
        }
    }

    return isSuccess;
#else
    UNREFERENCED_PARAMETER(params);
    //for windows we still do not have container group support
    return true;
#endif
}

bool FabricTestDispatcher::VerifyMoveCostValue(Common::StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::VerifyMoveCostValueCommand);
        return false;
    }

    // It takes time for the move cost to propagate to PLB, so we want to try up to 30 times (60sec) before failing.
    int retries = 0;
    int maxRetries = 30;

    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);
    TestSession::FailTestIf(!failoverUnit, "FailoverUnit {0} not found", params[0]);

    std::wstring serviceName = params[0];

    Reliability::LoadBalancingComponent::ReplicaRole::Enum role =
        (0 == StringUtility::CompareCaseInsensitive(params[1].c_str(), L"primary")) ?
        Reliability::LoadBalancingComponent::ReplicaRole::Primary :
        Reliability::LoadBalancingComponent::ReplicaRole::Secondary;

    uint value = Common::Int32_Parse(params[2]);
    TestSession::FailTestIfNot(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabled, "VerifyLoadValue called when DummyPLB not enabled");

    auto fm = GetFM();

    if (!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready");
        return false;
    }

    auto plb = fm->PLBTestHelper;
    TestSession::FailTestIf(!plb, "PLB not valid");

    bool moveCostVerified = false;

    while (!moveCostVerified && retries < maxRetries)
    {
        moveCostVerified = plb->CheckMoveCostValue(serviceName, failoverUnit->Id.Guid, role, value);
        if (!moveCostVerified)
        {
            TestSession::WriteWarning(TraceSource, "Move cost for service {0} role {1} not verified - retrying", serviceName, role);
            Sleep(2000);
        }
        retries++;
    }

    TestSession::FailTestIfNot(moveCostVerified, "VerifyMoveCostValue failed");

    return true;
}

bool FabricTestDispatcher::PromoteToPrimary(StringCollection const & params)
{
    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::SwapPrimaryCommand);
        return false;
    }

    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);

    std::wstring serviceName = params[0];

    TestSession::FailTestIf(!failoverUnit, "FailoverUnit {0} not found", params[0]);
    //We assert in the PLB that the current primary is the correct one
    NodeId newPrimary = ParseNodeId(params[1]);

    TestSession::FailTestIfNot(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabled, "SwapPrimary called when DummyPLB not enabled");

    auto fm = GetFM();

    if (!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready");
        return false;
    }

    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fm->PLB);
    TestSession::FailTestIf(!plb, "PLB not valid");
    plb->ProcessPendingUpdatesPeriodicTask(); //Called to make sure that the FM's updateversion is in sync with the PLB's -- timing issues can cause inconsistent results otherwise
    plb->TriggerPromoteToPrimary(serviceName, failoverUnit->Id.Guid, newPrimary);

    return true;
}

bool FabricTestDispatcher::ProcessPendingPlbUpdates(StringCollection const& params)
{
    UNREFERENCED_PARAMETER(params);

    auto fm = GetFM();
    if (!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM is not ready");
        return false;
    }

    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fm->PLB);
    TestSession::FailTestIf(!plb, "PLB is not valid");

    plb->ProcessPendingUpdatesPeriodicTask();

    return true;
}

bool FabricTestDispatcher::SwapPrimary(StringCollection const & params)
{
    int pSize = static_cast<int>(params.size());

    if (!(pSize == 3 || pSize == 4 || pSize == 5))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::SwapPrimaryCommand);
        return false;
    }

    if (pSize == 3)
        TestSession::WriteInfo(TraceSource, "Auto-Verifying PLB stability before executing SwapPrimary {0} {1} {2}", params[0], params[1], params[2]);
    else if (pSize == 4)
        TestSession::WriteInfo(TraceSource, "Auto-Verifying PLB stability before executing SwapPrimary {0} {1} {2} {3}", params[0], params[1], params[2], params[3]);
    else if (pSize == 5)
        TestSession::WriteInfo(TraceSource, "Skipping Auto-Verifying before executing SwapPrimary {0} {1} {2} {3} {4}", params[0], params[1], params[2], params[3], params[4]);

    auto capitalize = [](std::wstring str) -> std::wstring
    {
        StringUtility::ToUpper(str);
        return str;
    };

    bool skipVerify = false;
    bool forceOpt = false;
    
    for(auto const & param: params)
    {
        if (capitalize(param) == L"SKIPVERIFY")
        {
            skipVerify = true;
            continue;
        }
        if (capitalize(param) == L"FORCE")
        {
            forceOpt = true;
            continue;
        }
    }

    if (!skipVerify)
        VerifyAll(StringCollection(), true);

    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);

    std::wstring serviceName = params[0];

    TestSession::FailTestIf(!failoverUnit, "FailoverUnit {0} not found", params[0]);
    //We assert in the PLB that the current primary is the correct one


    NodeId currentPrimary = ParseNodeId(params[1]);
    NodeId newPrimary;
    bool chooseRandomly = false;

    if (capitalize(params[2]) == L"RANDOM")
    {
        newPrimary = ParseNodeId(params[1]);
        chooseRandomly = true;
    }
    else
    {
        newPrimary = ParseNodeId(params[2]);
    }

    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * plb = nullptr;

    if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName)
    {
        auto fmm = GetFMM();

        if (!fmm)
        {
            TestSession::WriteWarning(TraceSource, "FMM not ready");
            return false;
        }

        plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fmm->PLB);
    }
    else
    {
        auto fm = GetFM();

        if (!fm)
        {
            TestSession::WriteWarning(TraceSource, "FM not ready");
            return false;
        }

        plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fm->PLB);
    }

    TestSession::FailTestIf(!plb, "PLB not valid");
    plb->ProcessPendingUpdatesPeriodicTask(); //Called to make sure that the FM's updateversion is in sync with the PLB's -- timing issues can cause inconsistent results otherwise

    Common::ErrorCode result = plb->TriggerSwapPrimary(serviceName, failoverUnit->Id.Guid, currentPrimary, newPrimary, forceOpt, chooseRandomly);

    if (result.IsSuccess())
    {
        return true;
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "Could not execute SwapPrimary because it would violate: {0} -- {1}", result.ErrorCodeValueToString(), result.Message);
        return false;
    }
}

bool FabricTestDispatcher::MoveSecondary(StringCollection const & params)
{
    int pSize = static_cast<int>(params.size());

    if (!(pSize == 3 || pSize == 4))
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::MoveSecondaryCommand);
        return false;
    }

    if (pSize == 3)
    {
        TestSession::WriteInfo(TraceSource, "Auto-Verifying PLB Stability before executing MoveSecondary {0} {1} {2}", params[0], params[1], params[2]);
    }
    else if (pSize == 4)
    {
        TestSession::WriteInfo(TraceSource, "Auto-Verifying PLB Stability before executing MoveSecondary {0} {1} {2} {3}", params[0], params[1], params[2], params[3]);
    }


    VerifyAll(StringCollection(),true);

    auto failoverUnit = GetFMFailoverUnitFromParam(params[0]);
    TestSession::FailTestIf(!failoverUnit, "FailoverUnit {0} not found.", params[0]);

    std::wstring serviceName = params[0];



    //TestSession::FailTestIfNot(Reliability::LoadBalancingComponent::PLBConfig::GetConfig().DummyPLBEnabled, "MoveSecondary called when DummyPLB not enabled");
    auto capitalize = [](std::wstring str) -> std::wstring
    {
        StringUtility::ToUpper(str);
        return str;
    };

    NodeId currentSecondary = ParseNodeId(params[1]);
    NodeId newSecondary;
    bool chooseRandomly = false;

    if (capitalize(params[2]) == L"RANDOM")
    {
        newSecondary = ParseNodeId(params[1]);
        chooseRandomly = true;
    }
    else
    {
        newSecondary = ParseNodeId(params[2]);
    }

    bool forceOpt = (pSize < 4) ? false : (capitalize(params[3]) == L"FORCE");

    auto fm = GetFM();
    if (!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready.");
        return false;
    }

    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fm->PLB);

    TestSession::FailTestIf(!plb, "PLB not valid.");
    plb->ProcessPendingUpdatesPeriodicTask(); //Called to make sure that the FM's updateversion is in sync with the PLB's -- timing issues can cause inconsistent results otherwise
    Common::ErrorCode result = plb->TriggerMoveSecondary(serviceName, failoverUnit->Id.Guid, currentSecondary, newSecondary, forceOpt, chooseRandomly);

    if (result.IsSuccess())
    {
        return true;
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "Could not execute MoveSecondary because it would violate: {0} -- {1}", result.ErrorCodeValueToString(), result.Message);
        return false;
    }
}

bool FabricTestDispatcher::PLBUpdateService(StringCollection const& params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::PLBUpdateService);
        return false;
    }

    NamingUri serviceNameUri;
    if (!NamingUri::TryParse(params[0], serviceNameUri))
    {
        TestSession::WriteError(TraceSource, "Could not parse service name {0}.", params[0]);
        return false;
    }

    auto fm = GetFM();
    if (!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready.");
        return false;
    }

    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fm->PLB);

    TestSession::FailTestIf(!plb, "PLB not valid.");

    auto serviceSnapshot = fm->FindServiceByServiceName(serviceNameUri.ToString());
    TestSession::FailTestIf(!serviceSnapshot, "Service is null in PLBUpdateService().");

    auto serviceDescription = serviceSnapshot->GetPLBServiceDescription();

    CommandLineParser parser(params, 1);

    // force=true?
    bool forceUpdate = parser.GetBool(L"force");

    wstring parentService;
    if (!parser.TryGetString(L"affinity", parentService))
    {
        parentService = serviceDescription.AffinitizedService;
    }

    vector<Reliability::ServiceScalingPolicyDescription> scalingPolicies;
    wstring scalingPolicy;
    parser.TryGetString(L"scalingPolicy", scalingPolicy, L"");
    if (!scalingPolicy.empty())
    {
        if (!TestFabricClient::GetServiceScalingPolicy(scalingPolicy, scalingPolicies))
        {
            return false;
        }
    }

    Reliability::LoadBalancingComponent::ServiceDescription newDescription(
        wstring(serviceDescription.Name),
        wstring(serviceDescription.ServiceTypeName),
        wstring(serviceDescription.ApplicationName),
        serviceDescription.IsStateful,
        wstring(serviceDescription.PlacementConstraints),
        move(parentService),
        serviceDescription.AlignedAffinity,
        std::vector<Reliability::LoadBalancingComponent::ServiceMetric> (serviceDescription.Metrics),
        serviceDescription.DefaultMoveCost,
        serviceDescription.OnEveryNode,
        serviceDescription.PartitionCount,
        serviceDescription.TargetReplicaSetSize,
        true,
        ServiceModel::ServicePackageIdentifier(),
        ServiceModel::ServicePackageActivationMode::SharedProcess,
        0,
        move(scalingPolicies));

    auto error = plb->UpdateService(move(newDescription), forceUpdate);

    TestSession::FailTestIfNot(error.IsSuccess(), "UpdateService call to PLB failed with error {0}", error);

    return true;
}

bool FabricTestDispatcher::StopTestFabricClient()
{
    FabricClient.StopTestFabricClient();
    return true;
}

ITestStoreServiceSPtr FabricTestDispatcher::ResolveService(std::wstring const& serviceName, __int64 key, Common::Guid const& fuId, bool useDefaultNamedClient, bool allowAllError)
{
    ServiceLocationChangeClientSPtr client = nullptr;
    wstring cacheItemName(L"");
    if (useDefaultNamedClient)
    {
        clientManager_.GetDefaultNamedClient(client);
        StringWriter writer(cacheItemName);
        writer.Write("{0}-{1}", serviceName, key);
    }

    auto testStoreService = FabricClient.ResolveService(
        serviceName,
        FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64,
        reinterpret_cast<void const *>(&key),
        fuId,
        S_OK,
        false,
        client,
        cacheItemName,
        allowAllError);

    if (!testStoreService)
    {
        clientManager_.ResetDefaultNamedClient();
    }

    return testStoreService;
}

bool FabricTestDispatcher::ResolveService(
    StringCollection const & params,
    ServiceLocationChangeClientSPtr const & client)
{
    if (params.size() < 2 || params.size() > 6)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ResolveServiceCommand);
        return false;
    }

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring serviceName(name.ToString());

    TestSession::FailTestIf(IsTrue(params[1]),
        "Invalid parameter '{0}'", params[1]);

    CommandLineParser parser(params, 2);
    HRESULT expectedError = S_OK;
    wstring errorValue;
    if (parser.TryGetString(L"error", errorValue))
    {
        ErrorCodeValue::Enum error = ParseErrorCode(errorValue);
        expectedError = ErrorCode(error).ToHResult();
    }

    HRESULT expectedCompareVersionError = S_OK;
    wstring compareVersionErrorValue;
    if (parser.TryGetString(L"compareVersionError", compareVersionErrorValue))
    {
        ErrorCodeValue::Enum error = ParseErrorCode(compareVersionErrorValue);
        expectedCompareVersionError = ErrorCode(error).ToHResult();
    }

    // Force named partition even if name is integer
    bool namedPartition = parser.GetBool(L"namedpartition");

    bool savePartitionId = parser.GetBool(L"savecuid");
    bool verifyPartitionId = parser.GetBool(L"verifycuid");

    bool skipVerifyResults = parser.GetBool(L"skipverifyresults");

    bool noVerify = parser.GetBool(L"noverify");
    if (!noVerify)
    {
        // Need to wait for stabilization of GFUM before verifying resolve result.
        VerifyAll(StringCollection());
    }

    std::wstring cacheItemName;
    parser.TryGetString(L"cache", cacheItemName);

    ComPointer<IFabricServiceManagementClient> serviceClient;
    ServiceLocationChangeClientSPtr clientToUse = client;
    if (client)
    {
        serviceClient = client->Client;
    }

    if (!serviceClient)
    {
        wstring nodeIdString;
        if (parser.TryGetString(L"nodeconfig", nodeIdString, L""))
        {
            CreateFabricClientOnNode(nodeIdString, /*out*/serviceClient);
            clientToUse = make_shared<ServiceLocationChangeClient>(serviceClient);
        }
    }

    // No retry required because we make sure system is stable before calling Resolve
    __int64 key;
    if (IsFalse(params[1]))
    {
        FabricClient.ResolveService(
            serviceName,
            expectedError,
            clientToUse,
            cacheItemName,
            skipVerifyResults,
            expectedCompareVersionError);
    }
    else if (!namedPartition && Common::TryParseInt64(params[1], key))
    {
        FabricClient.ResolveService(
            serviceName,
            key,
            expectedError,
            clientToUse,
            cacheItemName,
            skipVerifyResults,
            expectedCompareVersionError);
    }
    else
    {
        FabricClient.ResolveService(
            serviceName,
            params[1],
            expectedError,
            clientToUse,
            cacheItemName,
            skipVerifyResults,
            savePartitionId,
            verifyPartitionId,
            expectedCompareVersionError);
    }

    return true;
}

bool FabricTestDispatcher::ResolveServiceUsingClient(StringCollection const & params)
{
    if (params.size() < 3 || params.size() > 9)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ResolveUsingClientCommand);
        return false;
    }

    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(params[0], client))
    {
        TestSession::WriteInfo(TraceSource, "Fabric client {0} not found", params[0]);
        return false;
    }

    const_cast<StringCollection&>(params).erase(params.begin());

    return ResolveService(params, client);
}

bool FabricTestDispatcher::GetServiceDescriptionUsingClient(StringCollection const & params)
{
    if (params.size() < 2 || params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::GetServiceDescriptionUsingClientCommand);
        return false;
    }

    ServiceLocationChangeClientSPtr client;
    if (!clientManager_.FindClient(params[0], client))
    {
        TestSession::WriteInfo(TraceSource, "Fabric client {0} not found", params[0]);
        return false;
    }

    const_cast<StringCollection&>(params).erase(params.begin());

    return FabricClient.GetServiceDescription(params, client->Client);
}

ITestStoreServiceSPtr FabricTestDispatcher::ResolveServiceWithEvents(std::wstring const& serviceName, bool isCommand, HRESULT expectedError)
{
    return FabricClient.ResolveServiceWithEvents(serviceName, FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE, NULL, Guid::Empty(), expectedError, isCommand);
}

ITestStoreServiceSPtr FabricTestDispatcher::ResolveServiceWithEvents(std::wstring const& serviceName, __int64 key, Guid const& fuId, bool isCommand, HRESULT expectedError)
{
    void const * keyPtr = reinterpret_cast<void const *>(&key);
    return FabricClient.ResolveServiceWithEvents(serviceName, FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64, keyPtr, fuId, expectedError, isCommand);
}

ITestStoreServiceSPtr FabricTestDispatcher::ResolveServiceWithEvents(std::wstring const& serviceName, std::wstring const & key, Guid const& fuId, bool isCommand, HRESULT expectedError)
{
    void const * keyPtr = reinterpret_cast<void const *>(key.c_str());
    return FabricClient.ResolveServiceWithEvents(serviceName, FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING, keyPtr, fuId, expectedError, isCommand);
}

bool FabricTestDispatcher::ResolveServiceWithEvents(StringCollection const & params)
{
    if (params.size() < 2 || params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ResolveServiceWithEventsCommand);
        return false;
    }

    NamingUri name;
    if (!NamingUri::TryParse(params[0], name))
    {
        TestSession::WriteError(
            TraceSource,
            "Could not parse name {0}.", params[0]);
        return false;
    }

    wstring serviceName(name.ToString());

    TestSession::FailTestIf(IsTrue(params[1]),
        "Invalid parameter '{0}'", params[1]);

    HRESULT expectedError = S_OK;
    if (params.size() == 3)
    {
        ErrorCodeValue::Enum error = ParseErrorCode(params[2]);
        expectedError = ErrorCode(error).ToHResult();
    }

    //Need to wait for stabilization of GFUM before verifying resolve result.
    VerifyAll(StringCollection());

    //No retry required because we make sure system is stable before calling Resolve
    __int64 key;
    if (IsFalse(params[1]))
    {
        ResolveServiceWithEvents(serviceName, true, expectedError);
    }
    else if (Common::TryParseInt64(params[1], key))
    {
        ResolveServiceWithEvents(serviceName, key, Guid::Empty(), true, expectedError);
    }
    else
    {
        ResolveServiceWithEvents(serviceName, params[1], Guid::Empty(), true, expectedError);
    }

    return true;
}

bool FabricTestDispatcher::PrefixResolveService(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    auto clientFactory = clientsTracker_.CreateCachedClientFactoryOnNode(parser);

    bool doAsync = parser.GetBool(L"async");

    if (doAsync)
    {
        return commandsTracker_.RunAsync([this, params, clientFactory]()
        {
            return FabricClient.PrefixResolveService(params, clientFactory);
        });
    }
    else
    {
        return FabricClient.PrefixResolveService(params, clientFactory);
    }
}

bool FabricTestDispatcher::ParseServiceGroupAddress(
    __in std::wstring const & serviceGroupAddress,
    __in std::wstring const & pattern,
    __out std::wstring & memberServiceAddress)
{
    size_t startPos = serviceGroupAddress.find(pattern);
    if (std::wstring::npos == startPos)
    {
        return false;
    }

    startPos += pattern.size();
    size_t endPos = serviceGroupAddress.find(FabricTestConstants::ServiceAddressDoubleDelimiter, startPos);
    memberServiceAddress = serviceGroupAddress.substr(startPos, std::wstring::npos == endPos ? endPos : endPos - startPos);
    Common::StringUtility::Replace(memberServiceAddress, FabricTestConstants::ServiceAddressEscapedDelimiter, FabricTestConstants::ServiceAddressDelimiter);

    return true;
}

bool FabricTestDispatcher::VerifyResolveResult(ResolvedServicePartition const& resolvedServiceLocations, wstring const& serviceName)
{
    auto fm = GetFM();
    if(!fm)
    {
        TestSession::WriteInfo(
            TraceSource,
            "FM not ready");
        return false;
    }

    auto gfum = fm->SnapshotGfum();

    vector<wstring> serviceLocationsPrimary;
    vector<wstring> serviceLocationsSecondary;
    vector<wstring> serviceLocationsNone;

    NamingUri namingUri;
    NamingUri::TryParse(serviceName, namingUri);
    if (!namingUri.Fragment.empty())
    {
        NamingUri serviceGroupUri;
        UriBuilder(namingUri).set_Fragment(L"").ToUri(serviceGroupUri);
        wstring serviceGroupName = serviceGroupUri.ToString();
        wstring pattern = FabricTestConstants::ServiceAddressDoubleDelimiter;
        pattern += serviceName;
        pattern += FabricTestConstants::MemberServiceNameDelimiter;

        for (auto fuIter = gfum.begin(); fuIter != gfum.end(); fuIter++)
        {
            if (fuIter->GetServiceInfo().Name == serviceGroupName)
            {
                auto replicas = fuIter->GetReplicas();
                for (auto replicaIter = replicas.cbegin(); replicaIter != replicas.cend(); replicaIter++)
                {
                    wstring memberServiceLocation;
                    if (!ParseServiceGroupAddress(replicaIter->ServiceLocation, pattern, memberServiceLocation))
                    {
                        return false;
                    }

                    if (replicaIter->IsCurrentConfigurationPrimary)
                    {
                        serviceLocationsPrimary.push_back(memberServiceLocation);
                    }
                    else if (replicaIter->IsCurrentConfigurationSecondary)
                    {
                        serviceLocationsSecondary.push_back(memberServiceLocation);
                    }
                    else
                    {
                        serviceLocationsNone.push_back(memberServiceLocation);
                    }
                }
            }
        }
    }
    else
    {
        for (auto fuIter = gfum.cbegin(); fuIter != gfum.end(); fuIter++)
        {
            if (fuIter->GetServiceInfo().Name == serviceName)
            {
                auto replicas = fuIter->GetReplicas();
                for (auto replicaIter = replicas.cbegin(); replicaIter != replicas.cend(); replicaIter++)
                {
                    if (replicaIter->IsCurrentConfigurationPrimary)
                    {
                        serviceLocationsPrimary.push_back(replicaIter->ServiceLocation);
                    }
                    else if (replicaIter->IsCurrentConfigurationSecondary)
                    {
                        serviceLocationsSecondary.push_back(replicaIter->ServiceLocation);
                    }
                    else
                    {
                        serviceLocationsNone.push_back(replicaIter->ServiceLocation);
                    }
                }
            }
        }
    }

    bool result = true;

    if (resolvedServiceLocations.IsStateful)
    {
        if (resolvedServiceLocations.Locations.ServiceReplicaSet.IsPrimaryLocationValid)
        {
            result = VerifyServiceLocation(serviceLocationsPrimary, resolvedServiceLocations.Locations.ServiceReplicaSet.PrimaryLocation);
            if (!result) { return false; }
        }

        for (wstring const & secondary : resolvedServiceLocations.Locations.ServiceReplicaSet.SecondaryLocations)
        {
            result = VerifyServiceLocation(serviceLocationsSecondary, secondary);
            if (!result) { return false; }
        }
    }
    else
    {
        for (wstring const & replica : resolvedServiceLocations.Locations.ServiceReplicaSet.ReplicaLocations)
        {
            result = VerifyServiceLocation(serviceLocationsNone, replica);
            if (!result) { return false; }
        }
    }

    return result;
}

bool FabricTestDispatcher::VerifyServiceLocation(vector<wstring> const & serviceLocations, wstring const & toCheck)
{
    auto it = find(serviceLocations.begin(), serviceLocations.end(), toCheck);
    bool result = (it != serviceLocations.end());
    if (!result)
    {
        TestSession::WriteNoise(TraceSource,
            "Could not find service location {0} in GFUM", toCheck);
    }

    return result;
}

bool FabricTestDispatcher::VerifyUpgradeApp(StringCollection const & params)
{
    CHECK_PARAMS(1, 999, FabricTestCommands::VerifyUpgradeAppCommand);

    wstring appName = params[0];
    TimeSpan timeout = FabricTestSessionConfig::GetConfig().VerifyTimeout;

    CommandLineParser parser(params, 1);

    wstring timeoutString;
    parser.TryGetString(L"timeout", timeoutString, L"");

    if (!timeoutString.empty())
    {
        double seconds;
        StringUtility::TryFromWString(timeoutString, seconds);
        timeout = TimeSpan::FromSeconds(seconds);
    }

    TimeoutHelper timeoutHelper(timeout);

    bool expectTimeout = parser.GetBool(L"expectTimeout");

    wstring upgradeDomainList;
    parser.TryGetString(L"upgradedomains", upgradeDomainList, L"");

    vector<wstring> upgradeDomains;
    StringUtility::Split<wstring>(upgradeDomainList, upgradeDomains, L",");

    FABRIC_ROLLING_UPGRADE_MODE upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
    wstring upgradeModeString;
    if (parser.TryGetString(L"upgrademode", upgradeModeString, L""))
    {
        if (upgradeModeString == L"manual")
        {
            upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
        }
        else if (upgradeModeString == L"monitored")
        {
            upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;
        }
        else if (upgradeModeString == L"auto")
        {
            upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
        }
    }

    vector<FABRIC_APPLICATION_UPGRADE_STATE> upgradeStates;
    wstring upgradeStatesCSV;
    if (parser.TryGetString(L"upgradestate", upgradeStatesCSV, L""))
    {
        vector<wstring> stateStrings;
        StringUtility::Split<wstring>(upgradeStatesCSV, stateStrings, L",");
        for (auto const & upgradeStateString : stateStrings)
        {
            auto upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_INVALID;

            if (upgradeStateString == L"rollbackinprogress")
            {
                upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS;
            }
            else if (upgradeStateString == L"rollbackcompleted")
            {
                upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_COMPLETED;
            }
            else if (upgradeStateString == L"rollforwardpending")
            {
                upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_PENDING;
            }
            else if (upgradeStateString == L"rollforwardinprogress")
            {
                upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS;
            }
            else if (upgradeStateString == L"rollforwardcompleted")
            {
                upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED;
            }
            else if (upgradeStateString == L"rollbackpending")
            {
                upgradeState = FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_PENDING;
            }

            if (upgradeState != FABRIC_APPLICATION_UPGRADE_STATE_INVALID)
            {
                upgradeStates.push_back(upgradeState);
            }
        }
    }

    DWORD interval = 1000;
    bool timedOut = false;
    while(!ApplicationMap.VerifyUpgrade(appName, upgradeDomains, upgradeMode, upgradeStates))
    {
        TestSession::WriteInfo(TraceSource, "Upgrade for {0} not yet complete {1}", appName, upgradeDomains);
        Sleep(interval);
        interval = min<DWORD>(interval + 500, 3000);

        if (timeoutHelper.IsExpired)
        {
            timedOut = true;
            break;
        }
    }

    if(timedOut != expectTimeout)
    {
        if(timedOut)
        {
            TestSession::FailTest(
                "Upgrade verification failed with a Timeout of {0} seconds.",
                 timeout.TotalSeconds());
        }
        else
        {
            TestSession::FailTest(
                "Upgrade verification did not timeout in {0} seconds as expected",
                 timeout.TotalSeconds());
        }
    }

    TestSession::WriteInfo(TraceSource, "Upgrade verification for {0} succeeded {1}", appName, upgradeDomains);

    return true;
}

bool FabricTestDispatcher::PrepareUpgradeFabric(StringCollection const & params)
{
    CHECK_PARAMS(1, 2, FabricTestCommands::PrepareUpgradeFabricCommand);

    CommandLineParser parser(params, 0);
    wstring codeVersion;
    wstring configVersion;
    parser.TryGetString(L"code", codeVersion);
    parser.TryGetString(L"config", configVersion);

    if (codeVersion.empty() && configVersion.empty())
    {
        TestSession::WriteError(TraceSource, "Must provide either code or config");
        return false;
    }

    wstring incomingDirectory(L"incoming");
    wstring pathToIncoming = Path::Combine(Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestImageStoreDirectory), incomingDirectory);
    if (!Directory::Exists(pathToIncoming))
    {
        Directory::Create(pathToIncoming);
    }

    wstring pathToPatch, pathToConfig;
    if (!codeVersion.empty())
    {
        pathToPatch = Path::Combine(pathToIncoming, Management::ImageModel::WinFabStoreLayoutSpecification::GetPatchFileName(codeVersion));
        Common::FileWriter patchFileWriter;
        auto error = patchFileWriter.TryOpen(pathToPatch);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(TraceSource, "Failed to open patch file {0}:{1}", pathToPatch, error);
            return false;
        }
        patchFileWriter << "arbitrary content";
        patchFileWriter.Close();
    }

    if (!configVersion.empty())
    {
        wstring pathToTemplate = Path::Combine(Environment::GetExecutablePath(), L"ClusterManifest-FabricUpgrade.xml");
        File fileReader;
        auto error = fileReader.TryOpen(pathToTemplate, FileMode::Open, FileAccess::Read, FileShare::ReadWrite);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(TraceSource, "read file {0} error {1}", pathToTemplate, error);
            return false;
        }
        size_t size = static_cast<size_t>(fileReader.size());
        string filetext;
        filetext.resize(size);
        fileReader.Read(&filetext[0], static_cast<int>(size));
        fileReader.Close();

        string configVersionAsc;
        StringUtility::UnicodeToAnsi(configVersion, configVersionAsc);
        string toReplace = "%configversion%";
        filetext.replace(filetext.find(toReplace), toReplace.length(), configVersionAsc);

        string infraTypeToReplace = "%infratype%";
#if !defined(PLATFORM_UNIX)
        string actualInfraType = "WindowsServer";
#else
        string actualInfraType = "Linux";
#endif
        filetext.replace(filetext.find(infraTypeToReplace), infraTypeToReplace.length(), actualInfraType);
        filetext.replace(filetext.find(infraTypeToReplace), infraTypeToReplace.length(), actualInfraType);

        pathToConfig = Path::Combine(pathToIncoming, Management::ImageModel::WinFabStoreLayoutSpecification::GetClusterManifestFileName(configVersion));
        Common::FileWriter configFileWriter;
        error = configFileWriter.TryOpen(pathToConfig);
        if (!error.IsSuccess())
        {
            TestSession::WriteError(TraceSource, "Failed to open config file {0}:{1}", pathToConfig, error);
            return false;
        }
        configFileWriter << filetext;
        configFileWriter.Close();
    }

    if (this->IsNativeImageStoreEnabled)
    {
        ComPointer<IFabricNativeImageStoreClient> clientCPtr;
        this->CreateNativeImageStoreClient(clientCPtr);

        if(!codeVersion.empty())
        {
            wstring relativeStoreLocationPatch = Path::Combine(incomingDirectory, Management::ImageModel::WinFabStoreLayoutSpecification::GetPatchFileName(codeVersion));
            this->UploadToNativeImageStore(
                pathToPatch,
                relativeStoreLocationPatch,
                clientCPtr);
        }

        if(!configVersion.empty())
        {
            wstring relativeStoreLocationConfig = Path::Combine(incomingDirectory, Management::ImageModel::WinFabStoreLayoutSpecification::GetClusterManifestFileName(configVersion));
            this->UploadToNativeImageStore(
                pathToConfig,
                relativeStoreLocationConfig,
                clientCPtr);
        }
    }

    return true;
}

bool FabricTestDispatcher::VerifyUpgradeFabric(StringCollection const & params)
{
    TimeSpan timeout = FabricTestSessionConfig::GetConfig().VerifyUpgradeTimeout;

    CommandLineParser parser(params, 0);

    wstring timeoutString;
    parser.TryGetString(L"timeout", timeoutString, L"");

    if (!timeoutString.empty())
    {
        double seconds;
        StringUtility::TryFromWString(timeoutString, seconds);
        timeout = TimeSpan::FromSeconds(seconds);
    }

    TimeoutHelper timeoutHelper(timeout);

    bool expectTimeout = parser.GetBool(L"expectTimeout");

    wstring upgradeDomainList;
    parser.TryGetString(L"upgradedomains", upgradeDomainList, L"");

    vector<wstring> upgradeDomains;
    StringUtility::Split<wstring>(upgradeDomainList, upgradeDomains, L",");

    FABRIC_ROLLING_UPGRADE_MODE upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
    wstring upgradeModeString;
    if (parser.TryGetString(L"upgrademode", upgradeModeString, L""))
    {
        if (upgradeModeString == L"manual")
        {
            upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
        }
        else if (upgradeModeString == L"monitored")
        {
            upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;
        }
        else if (upgradeModeString == L"auto")
        {
            upgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
        }
    }

    vector<FABRIC_UPGRADE_STATE> upgradeStates;
    wstring upgradeStatesCSV;
    if (parser.TryGetString(L"upgradestate", upgradeStatesCSV, L""))
    {
        vector<wstring> stateStrings;
        StringUtility::Split<wstring>(upgradeStatesCSV, stateStrings, L",");

        for (auto const & upgradeStateString : stateStrings)
        {
            auto upgradeState = FABRIC_UPGRADE_STATE_INVALID;

            if (upgradeStateString == L"rollbackinprogress")
            {
                upgradeState = FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS;
            }
            else if (upgradeStateString == L"rollbackcompleted")
            {
                upgradeState = FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED;
            }
            else if (upgradeStateString == L"rollforwardpending")
            {
                upgradeState = FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING;
            }
            else if (upgradeStateString == L"rollforwardinprogress")
            {
                upgradeState = FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS;
            }
            else if (upgradeStateString == L"rollforwardcompleted")
            {
                upgradeState = FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED;
            }
            else if (upgradeStateString == L"rollbackpending")
            {
                upgradeState = FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING;
            }

            if (upgradeState != FABRIC_UPGRADE_STATE_INVALID)
            {
                upgradeStates.push_back(upgradeState);
            }
        }
    }

    DWORD interval = 5000;
    bool timedOut = false;
    while(!UpgradeFabricData.VerifyUpgrade(upgradeDomains, upgradeMode, upgradeStates))
    {
        TestSession::WriteInfo(TraceSource, "Upgrade for Fabric not yet complete {0}", upgradeDomains);
        Sleep(interval);
        interval = min<DWORD>(interval + 500, 10000);

        if (timeoutHelper.IsExpired)
        {
            timedOut = true;
            break;
        }
    }

    if(timedOut != expectTimeout)
    {
        if(timedOut)
        {
            TestSession::FailTest(
                "Upgrade verification failed with a Timeout of {0} seconds.",
                 timeout.TotalSeconds());
        }
        else
        {
            TestSession::FailTest(
                "Upgrade verification did not timeout in {0} seconds as expected",
                 timeout.TotalSeconds());
        }
    }
    
    TestSession::WriteInfo(TraceSource, "Upgrade verification for Fabric succeeded {0}", upgradeDomains);
    return true;
}

bool FabricTestDispatcher::FailFabricUpgradeDownload(Common::StringCollection const & params)
{
    if(params.size() != 1)
    {
        TestSession::WriteWarning(TraceSource, "True/False parameter should be passed");
        return false;
    }

    bool shouldFail = IsTrue(params[0]);
    TestFabricUpgradeHostingImpl::ShouldFailDownload = shouldFail;

    return true;
}

bool FabricTestDispatcher::FailFabricUpgradeValidation(Common::StringCollection const & params)
{
    if(params.size() != 1)
    {
        TestSession::WriteWarning(TraceSource, "True/False parameter should be passed");
        return false;
    }

    bool shouldFail = IsTrue(params[0]);
    TestFabricUpgradeHostingImpl::ShouldFailValidation = shouldFail;

    return true;
}

bool FabricTestDispatcher::FailFabricUpgrade(Common::StringCollection const & params)
{
    if(params.size() != 1)
    {
        TestSession::WriteWarning(TraceSource, "True/False parameter should be passed");
        return false;
    }

    bool shouldFail = IsTrue(params[0]);
    TestFabricUpgradeHostingImpl::ShouldFailUpgrade = shouldFail;

    return true;
}

void FabricTestDispatcher::UploadToNativeImageStore(
    wstring const & localLocation,
    wstring const & storeRelativePath,
    ComPointer<IFabricNativeImageStoreClient> const & imageStoreClientCPtr)
{
    auto hr = imageStoreClientCPtr->UploadContent(
        storeRelativePath.c_str(),
        localLocation.c_str(),
        this->GetComTimeout(ServiceModelConfig::GetConfig().MaxFileOperationTimeout),
        FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC);

    TestSession::WriteInfo(
        TraceSource,
        "Upload to native image store '{0}' -> '{1}': hr = {2} ({3})",
        localLocation,
        storeRelativePath,
        hr,
        ErrorCode::FromHResult(hr));

    TestSession::FailTestIfNot(SUCCEEDED(hr), "UploadContent failed: hr = {0}", hr);
}

bool FabricTestDispatcher::InjectFailure(StringCollection const & params)
{
    if (params.size() != 3 &&
        params.size() != 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::InjectFailureCommand);
        return false;
    }

    ApiFaultHelper::Get().AddFailure(params[0], params[1], params[2]);

    if (params.size() == 4)
    {
        auto timeoutSeconds = Common::Int32_Parse(params[3]);
        auto root = FABRICSESSION.CreateComponentRoot();
        Threadpool::Post([this, root, params]()
        {
            ApiFaultHelper::Get().RemoveFailure(params[0], params[1], params[2]);
        },
        TimeSpan::FromSeconds(timeoutSeconds),
        false);
    }

    return true;
}

bool FabricTestDispatcher::RemoveFailure(StringCollection const & params)
{
    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveFailureCommand);
        return false;
    }

    wstring nodeIdStr = params[0];
    wstring serviceNameStr = params[1];

    ApiFaultHelper::Get().RemoveFailure(nodeIdStr, serviceNameStr, params[2]);
    return true;
}

bool SignalCommandRunner(
    std::wstring const & commandName,
    StringCollection const & params,
    std::function<void(std::wstring const &, std::wstring const &, std::wstring const &)> func)
{
    if (params.size() < 3)
    {
        FABRICSESSION.PrintHelp(commandName);
        return false;
    }

    func(params[0], params[1], params[2]);
    return true;
}

bool FabricTestDispatcher::SetSignal(StringCollection const & params)
{
    return SignalCommandRunner(
        FabricTestCommands::SetSignalCommand,
        params,
        [](wstring const & nodeId, wstring const & serviceName, wstring const & signal)
    {
        ApiSignalHelper::AddSignal(nodeId, serviceName, signal);
    });
}

bool FabricTestDispatcher::ResetSignal(StringCollection const & params)
{
    if (params.size() == 1 && params[0] == L"all")
    {
        ApiSignalHelper::ResetAllSignals();
        return true;
    }

    return SignalCommandRunner(
        FabricTestCommands::ResetSignalCommand,
        params,
        [](wstring const & nodeId, wstring const & serviceName, wstring const & signal)
    {
        ApiSignalHelper::ResetSignal(nodeId, serviceName, signal);
    });
}

bool FabricTestDispatcher::WaitForSignalHit(StringCollection const & params)
{
    return SignalCommandRunner(
        FabricTestCommands::ResetSignalCommand,
        params,
        [](wstring const & nodeId, wstring const & serviceName, wstring const & signal)
    {
        ApiSignalHelper::WaitForSignalHit(nodeId, serviceName, signal);
    });
}

bool FabricTestDispatcher::FailIfSignalHit(Common::StringCollection const & params)
{
    if (params.size() != 4)
    {
        TestSession::FailTest("Must have 4 params");
    }

    int64 seconds = 0;
    if (!TryParseInt64(params[3], seconds))
    {
        TestSession::FailTest("seconds must be int");
    }

    auto timeout = TimeSpan::FromSeconds(static_cast<double>(seconds));

    return SignalCommandRunner(
        FabricTestCommands::ResetSignalCommand,
        params,
        [timeout](wstring const & nodeId, wstring const & serviceName, wstring const & signal)
    {
        ApiSignalHelper::FailIfSignalHit(nodeId, serviceName, signal, timeout);
    });
}

bool FabricTestDispatcher::WaitAsync(StringCollection const & params)
{
    CommandLineParser parser(params);

    int timeoutSeconds = 0;
    parser.TryGetInt(L"timeout", timeoutSeconds, 60);

    return commandsTracker_.WaitForAll(TimeSpan::FromSeconds(timeoutSeconds));
}

bool FabricTestDispatcher::SetDefaultNodeCredentials(std::wstring const& value)
{
    defaultNodeCredentials_ = value == L"None" ? L"" : value;
    return true;
}

bool FabricTestDispatcher::SetClusterWideCredentials(std::wstring const& value)
{
    if(value == L"None")
    {
        clusterWideCredentials_.clear();
        ClearDefaultCertIssuers();
        return true;
    }

    StringCollection clusterWideCredentials;
    StringUtility::Split<wstring>(value, clusterWideCredentials, L":", false);

    if (StringUtility::CompareCaseInsensitive(clusterWideCredentials[0], L"Windows") == 0)
    {
        if (clusterWideCredentials.size() != 2)
        {
            TestSession::WriteError(TraceSource, "Invalid params for ClusterWideCredentials");
            return false;
        }

        clusterWideCredentials_ = clusterWideCredentials;
        ClearDefaultCertIssuers();
        return true;
    }

    if((clusterWideCredentials.size() != 5) && (clusterWideCredentials.size() != 8) && (clusterWideCredentials.size() != 9))
    {
        TestSession::WriteError(TraceSource, "Invalid params for ClusterWideCredentials");
        return false;
    }

    if(clusterWideCredentials[2] == L"Empty")
    {
        clusterWideCredentials[2] = L"";
    }

    if(clusterWideCredentials[3] == L"Empty")
    {
        clusterWideCredentials[3] = L"";
    }

    if(clusterWideCredentials[4] == L"Empty")
    {
        clusterWideCredentials[4] = L"";
    }

    clusterWideCredentials_ = clusterWideCredentials;
    SetDefaultCertIssuers();
    return true;
}

bool FabricTestDispatcher::SetClientCredentials(std::wstring const& value)
{
    wstring errorMessage;
    bool result = ClientSecurityHelper::TryCreateClientSecuritySettings(
        value,
        clientCredentialsType_, // out
        clientCredentials_, // out
        errorMessage); // out

    if (!result)
    {
        TestSession::WriteError(TraceSource, "SetClientCredentials failed: {0}", errorMessage);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "SetClientCredentials result: {0}", clientCredentials_);
    }

    return result;
}

ReplicaSnapshotUPtr FabricTestDispatcher::GetReplicaFromParam(wstring const& param, FailoverUnitSnapshotUPtr & failoverUnit)
{
    //param:servicename#partition_index#replica_index
    ReplicaSnapshotUPtr replica;
    wstring::size_type lastIndex =  param.find_last_of(FabricTestDispatcher::ServiceFUDelimiter);
    wstring partitionIdString = param.substr(0, lastIndex);
    failoverUnit = FABRICSESSION.FabricDispatcher.GetFMFailoverUnitFromParam(partitionIdString);

    TestSession::WriteInfo(TraceSource, "in GetReplicaFromParam failoverUnit is {0}", failoverUnit);

    int replicaIndex = Common::Int32_Parse(param.substr(lastIndex + 1));
    TestSession::FailTestIf(replicaIndex < 0, "replicaIndex should not be less than 0");

    int count = 0;
    auto replicas = failoverUnit->GetReplicas();
    for (auto iter = replicas.cbegin(); iter != replicas.cend(); ++iter)
    {
        if (count == replicaIndex)
        {
            TestSession::WriteInfo(TraceSource, "in GetReplicaFromParam replica is {0}", *iter);
            return make_unique<ReplicaSnapshot>(*iter);
        }

        count++;
    }

    TestSession::FailTest("replicaIndex not found for FT");
}

FailoverUnitSnapshotUPtr FabricTestDispatcher::GetFMFailoverUnitFromParam(wstring const& param)
{
    int fuIndex = 0;
    wstring serviceName = param;
    wstring::size_type index =  param.find(FabricTestDispatcher::ServiceFUDelimiter);
    if(index != wstring::npos)
    {
        serviceName = param.substr(0, index);
        fuIndex =  Common::Int32_Parse(param.substr(index + 1, param.size() - (index + 1)));
    }

    TestSession::FailTestIf(fuIndex < 0, "fuIndex should not be less than 0");
    if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName)
    {
        if (FailoverConfig::GetConfig().TargetReplicaSetSize > 0)
        {
            auto fmm = GetFMM();
            if(!fmm)
            {
                return nullptr;
            }

            return fmm->FindFTByPartitionId(Reliability::Constants::FMServiceGuid);
        }
    }
    else if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalRepairManagerServiceName ||
             StringUtility::StartsWith(serviceName, *ServiceModel::SystemServiceApplicationNameHelper::InternalInfrastructureServiceName))
    {
        // TODO Talk to Anmol about if there's a better way to do this.
        // Or change RunCommand to take a public service name instead of a partition ID, and then
        // have the naming gateway do the partition lookup.

        auto fm = GetFM();
        if(!fm)
        {
            return nullptr;
        }

        auto fts = fm->FindFTByServiceName(serviceName);
        if (fts.empty())
        {
            return FailoverUnitSnapshotUPtr();
        }
        else
        {
            return fts[0].CloneToUPtr();
        }
    }
    else
    {
        Guid fuId = ServiceMap.GetFUIdAt(serviceName, fuIndex);
        auto fm = GetFM();
        if(!fm)
        {
            return nullptr;
        }

        return fm->FindFTByPartitionId(fuId);
    }
    return nullptr;
}

ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr FabricTestDispatcher::GetRAFailoverUnitFromParam(
    wstring const& param,
    NodeId nodeId)
{
    int fuIndex = 0;
    wstring serviceName = param;
    wstring::size_type index =  param.find(FabricTestDispatcher::ServiceFUDelimiter);
    if(index != wstring::npos)
    {
        serviceName = param.substr(0, index);
        fuIndex =  Common::Int32_Parse(param.substr(index + 1, param.size() - (index + 1)));
    }

    TestSession::FailTestIf(fuIndex < 0, "fuIndex should be greater than 0");

    Guid fuId = ServiceMap.GetFUIdAt(serviceName, fuIndex);

    FabricTestNodeSPtr fabricTestNode = testFederation_.GetNode(nodeId);

    // Force error if node not found
    TestSession::FailTestIfNot(fabricTestNode != nullptr, "Node {0} does not exist", nodeId);

    ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr rv;

    fabricTestNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
    {
        auto ra = reliabilityTestHelper.GetRA();

        rv = move(ra->FindFTByPartitionId(fuId));
    });

    return rv;
}

bool FabricTestDispatcher::SetProperty(StringCollection const & params)
{
    if (configurationSetter_.SetProperty(params))
    {
        return true;
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"DefaultNodeCredentials"))
    {
        return SetDefaultNodeCredentials(params[1]);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"ClusterWideCredentials"))
    {
        return SetClusterWideCredentials(params[1]);
    }
    else if (StringUtility::AreEqualCaseInsensitive(params[0], L"ClientCredentials"))
    {
        return SetClientCredentials(params[1]);
    }
    else
    {
        TestSession::WriteError(TraceSource, "Cannot set unrecognized config parameter '{0}'", params[0]);
        return false;
    }
}

bool FabricTestDispatcher::SetFailoverManagerServiceProperties(StringCollection const & params)
{
    TestSession::FailTestIfNot(Federation.Count == 0, "Federation is not empty:{0}", Federation.Count);

    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineFailoverManagerServiceCommand);
        return false;
    }

    FailoverConfig::GetConfig().TargetReplicaSetSize = _wtoi(params[0].c_str());
    FailoverConfig::GetConfig().MinReplicaSetSize = _wtoi(params[1].c_str());

    return true;
}

bool FabricTestDispatcher::SetNamingServiceProperties(StringCollection const & params)
{
    TestSession::FailTestIfNot(Federation.Count == 0, "Federation is not empty:{0}", Federation.Count);

    if (params.size() != 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineNamingServiceCommand);
        return false;
    }

    NamingConfig & namingConfig = NamingConfig::GetConfig();
    namingConfig.PartitionCount = Int32_Parse(params[0]);
    namingConfig.TargetReplicaSetSize = Int32_Parse(params[1]);
    namingConfig.MinReplicaSetSize = Int32_Parse(params[2]);

    return true;
}

bool FabricTestDispatcher::SetClusterManagerServiceProperties(StringCollection const & params)
{
    TestSession::FailTestIfNot(Federation.Count == 0, "Federation is not empty:{0}", Federation.Count);

    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineClusterManagerServiceCommand);
        return false;
    }

    Management::ManagementConfig & managementConfig = Management::ManagementConfig::GetConfig();

    int targetReplicaSetSize = Int32_Parse(params[0]);
    int minReplicaSetSize = Int32_Parse(params[1]);

    managementConfig.TargetReplicaSetSize = targetReplicaSetSize;
    managementConfig.MinReplicaSetSize = minReplicaSetSize;

    // By default set ImageStoreService replica information to be the same as CM
    SetImageStoreServiceProperties(params);

    return true;
}

bool FabricTestDispatcher::SetImageStoreServiceProperties(StringCollection const & params)
{
    TestSession::FailTestIfNot(Federation.Count == 0, "Federation is not empty:{0}", Federation.Count);

    if (params.size() != 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineImageStoreServiceCommand);
        return false;
    }

    Management::ImageStore::ImageStoreServiceConfig & config =  Management::ImageStore::ImageStoreServiceConfig::GetConfig();

    int targetReplicaSetSize = Int32_Parse(params[0]);
    int minReplicaSetSize = Int32_Parse(params[1]);

    config.TargetReplicaSetSize = targetReplicaSetSize;
    config.MinReplicaSetSize = minReplicaSetSize;

    return true;
}

bool FabricTestDispatcher::SetRepairManagerServiceProperties(StringCollection const & params)
{
    CommandLineParser parser(params);

    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineRepairManagerServiceCommand);
        return false;
    }

    bool allowUpgrade = parser.GetBool(L"upgrade");

    TestSession::FailTestIfNot(
        allowUpgrade || Federation.Count == 0,
        "Federation is not empty:{0}; use 'upgrade' flag", Federation.Count);

    Management::RepairManager::RepairManagerConfig & config = Management::RepairManager::RepairManagerConfig::GetConfig();

    int targetReplicaSetSize = Int32_Parse(params[0]);
    int minReplicaSetSize = Int32_Parse(params[1]);

    config.TargetReplicaSetSize = targetReplicaSetSize;
    config.MinReplicaSetSize = minReplicaSetSize;

    if (config.TargetReplicaSetSize > 0)
    {
        repairManagerServiceDescription_ = Management::ManagementSubsystem::CreateRepairManagerServiceDescription();

        PartitionedServiceDescriptor psd;
        TestSession::FailTestIfNot(PartitionedServiceDescriptor::Create(repairManagerServiceDescription_, psd).IsSuccess(), "Failed to create RM PSD");

        ServiceMap.AddPartitionedServiceDescriptor(repairManagerServiceDescription_.Name, move(psd), true);
    }

    return true;
}

bool FabricTestDispatcher::SetInfrastructureServiceProperties(StringCollection const & params)
{
    TestSession::FailTestIfNot(Federation.Count == 0, "Federation is not empty:{0}", Federation.Count);

    if (params.size() < 2 || params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineInfrastructureServiceCommand);
        return false;
    }

    // Section name is dynamic, so config updates need to go directly to the config store,
    // rather than being set through the ManagementConfig object.
    wstring section = L"InfrastructureService";
    if (params.size() >= 3)
    {
        // Append instance name
        section.append(L"/");
        section.append(params[2]);
    }

    wstring setTarget = wformatString(
        "{0}.{1}={2}",
        section,
        L"TargetReplicaSetSize",
        params[0]);

    wstring setMin = wformatString(
        "{0}.{1}={2}",
        section,
        L"MinReplicaSetSize",
        params[1]);

    StringCollection settings;
    settings.push_back(setTarget);
    settings.push_back(setMin);

    TestConfigStore* configStore = dynamic_cast<TestConfigStore*>(Config::Test_DefaultConfigStore().get());
    bool result = configStore->UpdateConfig(settings);

    return result;
}

bool FabricTestDispatcher::RemoveInfrastructureServiceProperties(StringCollection const & params)
{
    if (params.size() > 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RemoveInfrastructureServiceCommand);
        return false;
    }

    wstring section = L"InfrastructureService";
    if (params.size() == 1)
    {
        // Append instance name
        section.append(L"/");
        section.append(params[0]);
    }

    TestConfigStore* configStore = dynamic_cast<TestConfigStore*>(Config::Test_DefaultConfigStore().get());
    bool result = configStore->RemoveSection(section);

    return result;
}

bool FabricTestDispatcher::SetDnsServiceProperties(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineDnsServiceCommand);
        return false;
    }

    DNS::DnsServiceConfig & config = DNS::DnsServiceConfig::GetConfig();
    config.IsEnabled = StringUtility::AreEqualCaseInsensitive(params[0], L"true");

    if (params.size() > 1)
    {
        config.EnablePartitionedQuery = StringUtility::AreEqualCaseInsensitive(params[1], L"true");
    }

    return true;
}

bool FabricTestDispatcher::SetNIMServiceProperties(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineDnsServiceCommand);
        return false;
    }

    auto & config = NetworkInventoryManager::NetworkInventoryManagerConfig::GetConfig();
    config.IsEnabled = StringUtility::AreEqualCaseInsensitive(params[0], L"true");

    return true;
}

bool FabricTestDispatcher::SetEnableUnsupportedPreviewFeatures(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineEnableUnsupportedPreviewFeaturesCommand);
        return false;
    }

    auto & config = CommonConfig::GetConfig();
    config.EnableUnsupportedPreviewFeatures = StringUtility::AreEqualCaseInsensitive(params[0], L"true");

    return true;
}

bool FabricTestDispatcher::SetMockImageBuilderProperties(StringCollection const & params)
{
    if (params.size() != 4)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::DefineMockImageBuilderCommand);
        return false;
    }

    auto cm = GetCM();
    if (!cm)
    {
        TestSession::WriteInfo(TraceSource, "CM not valid");
        return false;
    }

    wstring appName = params[0];
    wstring appBuildPath = params[1];
    wstring appTypeName = params[2];
    wstring appTypeVersion = params[3];

    return cm->SetMockImageBuilderParameter(
        appName,
        move(appBuildPath),
        move(appTypeName),
        move(appTypeVersion));
}

bool FabricTestDispatcher::EraseMockImageBuilderProperties(wstring const & appName)
{
    auto cm = GetCM();
    if (!cm)
    {
        TestSession::WriteInfo(TraceSource, "CM not valid");
        return false;
    }

    cm->EraseMockImageBuilderParameter(appName);
    return true;
}

bool FabricTestDispatcher::ShowLoadMetricInfo(StringCollection const &)
{
    auto fm = GetFM();
    if (fm)
    {
        wstring buf;
        StringWriter writer(buf);

        auto loads = fm->SnapshotLoadCache();
        for(auto it = loads.cbegin(); it != loads.cend(); ++it)
        {
            writer.WriteLine("{0}", *it);
        }

        TestSession::WriteInfo(TraceSource, "LoadMetrinInfo:\n{0}", buf);
        return true;
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "FM is not ready");
        return false;
    }
}

bool FabricTestDispatcher::ShowActiveServices(StringCollection const & params)
{
    if (params.size() > 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ShowActiveServicesCommand);
        return false;
    }

    if (params.size() == 1)
    {
        NodeId nodeId = ParseNodeId(params[0]);
        FabricTestNodeSPtr const & node = testFederation_.GetNode(nodeId);
        if (!node)
        {
            TestSession::WriteError(
                TraceSource,
                "Node {0} does not exist.", nodeId);
            return false;
        }

        node->PrintActiveServices();
    }
    else
    {
        for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
        {
            node->PrintActiveServices();
        }
    }

    return true;
}

bool FabricTestDispatcher::ListNodes()
{
    vector<NodeId> deactivatedNodes;
    wstring result;
    StringWriter writer(result);

    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        writer.Write("{0} ", node->Id);

        if (!node->IsActivated)
        {
            deactivatedNodes.push_back(node->Id);
        }
    }

    TestSession::WriteInfo(TraceSource, "{0} Nodes: {1}", testFederation_.Count, result);

    // Print out deactivated nodes
    wstring deactivatedNodesStr;
    StringWriter deactivatedNodesWriter(deactivatedNodesStr);
    for(auto it = deactivatedNodes.cbegin(); it != deactivatedNodes.cend(); ++it)
    {
        deactivatedNodesWriter.Write("{0} ", *it);
    }

    TestSession::WriteInfo(TraceSource, "{0} Deactivated: {1}", deactivatedNodes.size(), deactivatedNodesStr);

    return true;
}

bool FabricTestDispatcher::UpdateServicePackageVersionInstance(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        TestSession::WriteError(TraceSource, "Unknown params");
        return false;
    }

    NodeId nodeId = ParseNodeId(params[0]);
    wstring serviceName = params[1];

    auto fm = GetFM();
    if (fm == nullptr)
    {
        TestSession::WriteError(TraceSource, "FM not found");
        return false;
    }

    auto fts = fm->FindFTByServiceName(serviceName);
    if (fts.empty())
    {
        TestSession::WriteError(TraceSource, "FT not found");
        return false;
    }

    auto ft = fts[0];

    // Set it to 1.0:1.0:0
    RolloutVersion version(1, 0);
    ApplicationVersion appVersion(version);
    ServicePackageVersion packageVersion(appVersion, version);
    ServicePackageVersionInstance spvi(packageVersion, 0);
    fm->SetPackageVersionInstance(ft.Id, nodeId, spvi);

    return true;
}

bool FabricTestDispatcher::ShowFM()
{
    FabricTestNodeSPtr const & fmNode = testFederation_.GetNodeClosestToZero();
    if (fmNode)
    {
        TestSession::WriteInfo(TraceSource, "FM node is {0}", fmNode->Id);
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "No node in the federation");
    }

    return true;
}

FailoverManagerTestHelperUPtr FabricTestDispatcher::GetFMM()
{
    FabricTestNodeSPtr const & fmNode = testFederation_.GetNodeClosestToZero();
    if (!fmNode)
    {
        return nullptr;
    }

    auto fm = fmNode->Node->GetFailoverManagerMaster();

    if (fm == nullptr || !fm->IsReady)
    {
        return nullptr;
    }

    return fm;
}

FailoverManagerTestHelperUPtr FabricTestDispatcher::GetFM()
{
    FabricTestNodeSPtr const & fmNode = testFederation_.GetNodeClosestToZero();
    if (!fmNode)
    {
        return nullptr;
    }
    
    FailoverManagerTestHelperUPtr fmPtr;
    testFederation_.ForEachFabricTestNode([&](FabricTestNodeSPtr const& testNode)
    {
        auto fm = testNode->Node->GetFailoverManager();

        if (fm != nullptr && fm->IsReady)
        {
            fmPtr = move(fm);
            return;
        }
    });

    return fmPtr;
}

bool FabricTestDispatcher::ShowNodes()
{
    auto fm = GetFM();
    if (fm)
    {
        auto nodes = fm->SnapshotNodeCache();

        wstring result;
        StringWriter writer(result);

        for(auto it = nodes.cbegin(); it != nodes.cend(); ++it)
        {
            writer.WriteLine("{0}", *it);
        }

        TestSession::WriteInfo(TraceSource, "NodeTable:\n{0}", result);
        return true;
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "FM is not ready");
        return false;
    }
}

bool FabricTestDispatcher::ShowGFUM(StringCollection const & params)
{
    vector<FailoverUnitSnapshot> failoverUnits;
    vector<InBuildFailoverUnitSnapshot> inBuildFailoverUnits;

    if (testFederation_.Count == 0)
    {
        //TODO: Read GFUM from the location of the FMService when there are no nodes
        TestSession::WriteWarning(TraceSource, "Federation has 0 nodes up.");
        return true;
    }
    else
    {
        if (params.size() > 1)
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::ShowGFUMCommand);

            return false;
        }

        wstring param = params.size() == 1 ? params[0] : L"";
        FailoverManagerTestHelperUPtr fm;
        if(param == L"fmm")
        {
            fm = GetFMM();
        }
        else if(param == L"fm" || param.empty())
        {
            fm = GetFM();
        }
        else
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::ShowGFUMCommand);

            return false;
        }

        if (fm)
        {
            failoverUnits = fm->SnapshotGfum();
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "FM is not ready");
            return false;
        }

        inBuildFailoverUnits = fm->SnapshotInBuildFailoverUnits();
        auto services = fm->SnapshotServiceCache();
        auto serviceTypes = fm->SnapshotServiceTypes();
        auto applications = fm->SnapshotApplications();

        TestSession::WriteInfo(TraceSource,
            "FailoverUnits: {0}, InBuild FailoverUnits: {1}, services: {2}, service types: {3}, applications: {4}",
            failoverUnits.size(), inBuildFailoverUnits.size(), services.size(), serviceTypes.size(), applications.size());
    }

    wstring result;
    StringWriter writer(result);
    size_t count = 0;

    writer.WriteLine("Failover Units:");
    for(auto it = failoverUnits.cbegin(); it != failoverUnits.cend(); ++it)
    {
        writer.WriteLine("{0}", *it);
        count++;
        if (count >= 30)
        {
            writer.WriteLine("More failover units...");
            break;
        }
    }

    if (inBuildFailoverUnits.size() > 0)
    {
        count = 0;
        writer.WriteLine("In-Build Failover Units:");
        for(auto it = inBuildFailoverUnits.cbegin(); it != inBuildFailoverUnits.cend(); ++it)
        {
            writer.WriteLine("{0}", *it);
            count++;
            if (count >= 30)
            {
                writer.WriteLine("More in-build failover units...");
                break;
            }
        }
    }

    TestSession::WriteInfo(TraceSource, "{0}", result);

    return true;
}

bool FabricTestDispatcher::ShowLFUM(StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::ShowLFUMCommand);

        return false;
    }

    wstring firstParam = params[0];
    wstring fuid = params.size() > 1 ? params[1] : L"";

    vector<FabricTestNodeSPtr> nodes;

    if (StringUtility::AreEqualCaseInsensitive(firstParam, L"all"))
    {
        if (testFederation_.Count == 0)
        {
            TestSession::WriteWarning(TraceSource, "No nodes");
            return false;
        }

        for (FabricTestNodeSPtr const & testNode : testFederation_.GetNodes())
        {
            nodes.push_back(testNode);
        }
    }
    else
    {
        Common::StringCollection tokens;

        StringUtility::Split<wstring>(firstParam, tokens, L":");

        wstring nodeIdStr;

        if (tokens.size() == 2)
        {
            nodeIdStr = tokens[0];
        }
        else
        {
            nodeIdStr = firstParam;
        }

        NodeId nodeId = ParseNodeId(nodeIdStr);

        FabricTestNodeSPtr const & testNode = testFederation_.GetNode(nodeId);

        if (testNode == nullptr)
        {
            TestSession::WriteInfo(TraceSource, "Node {0} not found", nodeId);
            return false;
        }

        nodes.push_back(testNode);
    }

    for(auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        auto & testNode = *it;
        testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
        {
            auto ra = reliabilityTestHelper.GetRA();
            ShowLFUMOnNode(*ra, fuid);
        });
    }

    return true;
}

void FabricTestDispatcher::ShowLFUMOnNode(ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper & ra, wstring const & )
{
    if (!ra.IsOpen)
    {
        TestSession::WriteInfo(TraceSource, "RA is not Open");
        return;
    }

    auto failoverUnits = ra.SnapshotLfum();

    wstring result;
    StringWriter writer(result);

    size_t count = 0;
    for(auto it = failoverUnits.cbegin(); it != failoverUnits.cend(); ++it)
    {
        writer.WriteLine("{0}", *it);
        count++;
        if (count >= 30)
        {
            writer.WriteLine("More failover units...");
            break;
        }
    }

    TestSession::WriteInfo(TraceSource, "LFUM on {0}, {1} failover units:\n{2}", ra.NodeId, failoverUnits.size(), result);
}

bool FabricTestDispatcher::ShowService()
{
    vector<ServiceSnapshot> services;

    if (testFederation_.Count == 0)
    {
        //TODO: Read GFUM from the location of the FMService when there are no nodes
        TestSession::WriteWarning(TraceSource, "Federation has 0 nodes up.");
        return true;
    }
    else
    {
        auto fm = GetFM();
        if (fm)
        {
            services = fm->SnapshotServiceCache();
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "FM is not ready");
            return false;
        }
    }

    wstring result;
    StringWriter writer(result);

    for(auto service = services.cbegin(); service != services.cend(); ++service)
    {
        writer.WriteLine("{0}", *service);
    }

    TestSession::WriteInfo(TraceSource, "Service Table:\n{0}", result);

    return true;
}

bool FabricTestDispatcher::ShowServiceType()
{
    vector<ServiceTypeSnapshot> types;

    if (testFederation_.Count == 0)
    {
        //TODO: Read GFUM from the location of the FMService when there are no nodes
        TestSession::WriteWarning(TraceSource, "Federation has 0 nodes up.");
        return true;
    }
    else
    {
        auto fm = GetFM();
        if (fm)
        {
            types = fm->SnapshotServiceTypes();
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "FM is not ready");
            return false;
        }
    }

    wstring result;
    StringWriter writer(result);

    for(auto type = types.cbegin(); type != types.cend(); ++type)
    {
        writer.WriteLine("{0}\r\n", *type);
    }

    TestSession::WriteInfo(TraceSource, "{0}", result);

    return true;
}

bool FabricTestDispatcher::ShowApplication()
{
    vector<ApplicationSnapshot> applications;

    if (testFederation_.Count == 0)
    {
        //TODO: Read GFUM from the location of the FMService when there are no nodes
        TestSession::WriteWarning(TraceSource, "Federation has 0 nodes up.");
        return true;
    }
    else
    {
        auto fm = GetFM();
        if (fm)
        {
            applications = fm->SnapshotApplications();
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "FM is not ready");
            return false;
        }
    }

    wstring result;
    StringWriter writer(result);

    for(auto application = applications.cbegin(); application != applications.cend(); ++application)
    {
        writer.WriteLine("{0}\r\n", *application);
    }

    TestSession::WriteInfo(TraceSource, ":\n{0}", result);

    return true;
}

// Finds the node where CM primary replica is
shared_ptr<Management::ClusterManager::ClusterManagerReplica> FabricTestDispatcher::GetCM()
{
    // Find location of node where ClusterManager's primary replica is running.
    shared_ptr<FabricNodeWrapper> nodeWrapper =
        GetPrimaryNodeWrapperForService(ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName);

    // Get CM
    std::shared_ptr<Management::ClusterManager::ClusterManagerReplica> cm;

    if(nodeWrapper)
    {
        cm = nodeWrapper->GetCM();
    }

    if (!nodeWrapper || !cm)
    {
        TestSession::WriteWarning(TraceSource, "Could not get CM, Its not ready.");
    }

    return cm;
}


HealthManagerReplicaSPtr FabricTestDispatcher::GetHM()
{
    // Find location of node where ClusterManager's primary replica is running.
    shared_ptr<FabricNodeWrapper> nodeWrapper =
        GetPrimaryNodeWrapperForService(ServiceModel::SystemServiceApplicationNameHelper::InternalClusterManagerServiceName);

    // Get HM
    HealthManagerReplicaSPtr hm;
    if(nodeWrapper)
    {
        hm = nodeWrapper->GetHM();
    }
    else
    {
        TestSession::WriteInfo(TraceSource, "HM node not ready");
    }

    return hm;
}

//  Returns NodeWrapper object for the primary node where service' primary node is located.
// NULL on failure
shared_ptr<FabricNodeWrapper> FabricTestDispatcher::GetPrimaryNodeWrapperForService(wstring const& serviceName)
{
    // Create query string
    StringCollection findFULocationQuery;
    findFULocationQuery.push_back(L"Primary");
    findFULocationQuery.push_back(serviceName);

    // Get the primary's replica location/nodeId
    wstring servicePrimaryReplicaLocation = GetFailoverUnitLocationState(findFULocationQuery);

    // Parse nodeId from query result and create FabricNodeWrapper of that node
    shared_ptr<FabricNodeWrapper> nodeWrapper;
    if (servicePrimaryReplicaLocation.size() > 0)
    {
        NodeId nodeId;
        NodeId::TryParse(servicePrimaryReplicaLocation, nodeId);
        FabricTestNodeSPtr testNode = testFederation_.GetNode(nodeId);
        if(testNode)
        {
            testNode->TryGetFabricNodeWrapper(nodeWrapper);
        }
    }

    if(!nodeWrapper)
    {
        TestSession::WriteInfo(TraceSource, "Could not find primary replica for the service: '{0}'", serviceName);
    }

    // Done
    return nodeWrapper;
}

bool FabricTestDispatcher::ShowHM(StringCollection const & params)
{
    HealthManagerReplicaSPtr hm = GetHM();
    if (!hm)
    {
        return true;
    }

    return fabricClientHealth_->ShowHM(hm, params);
}

void FabricTestDispatcher::Verify()
{
    this->VerifyAll(StringCollection(), false);
}

bool FabricTestDispatcher::VerifyAll(StringCollection const & params, bool verifyPLB)
{
    TimeSpan timeout, apiFaultDisableTimeout;
    if (!params.empty())
    {
        double seconds;
        StringUtility::TryFromWString(params[0], seconds);
        timeout = TimeSpan::FromSeconds(seconds);
    }
    else
    {
        timeout = FabricTestSessionConfig::GetConfig().VerifyTimeout;
    }

    // Setup code to restore things to proper state
    bool isLoadBalancingEnabled = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled;
    bool isConstraintCheckEnabled =  Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintCheckEnabled;
    bool areRandomApiFaultsEnabled = ApiFaultHelper::Get().AreRandomApiFaultsEnabled;
    KFinally([isLoadBalancingEnabled, isConstraintCheckEnabled, areRandomApiFaultsEnabled]()
    {
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled = isLoadBalancingEnabled;
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintCheckEnabled = isConstraintCheckEnabled;
        ApiFaultHelper::Get().SetRandomApiFaultsEnabled(areRandomApiFaultsEnabled);
    });

    // Disable PLB and API faults during verify
    Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled = false;
    Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintCheckEnabled = false;


    fabricClientHealth_->HealthTable->Pause();

    TimeoutHelper timeoutHelper(timeout);
    TimeoutHelper apiFaultDisableTimeoutHelper(TimeSpan::FromSeconds(timeout.TotalSeconds() * FabricTestSessionConfig::GetConfig().RatioOfVerifyDurationWhenFaultsAreAllowed));

    CommandLineParser parser(params, 1);

    bool verifyEventsOnly = parser.GetBool(L"eventsonly");

    FABRICSESSION.WaitForExpectationEquilibrium(timeout);

    if (verifyEventsOnly)
    {
        return true;
    }

    DWORD interval = 1000;

    bool verifyFMMOnly = parser.GetBool(L"fm");

    int expectedFMReplicaCount;
    parser.TryGetInt(L"fmreplicas", expectedFMReplicaCount, -1);

    while (!(verifyFMMOnly ? (VerifyFMM(expectedFMReplicaCount) && VerifyFM()) : VerifyAll(verifyPLB)))
    {
        if (apiFaultDisableTimeoutHelper.IsExpired)
        {
            ApiFaultHelper::Get().SetRandomApiFaultsEnabled(false);
        }

        if (timeoutHelper.IsExpired)
        {
            TestSession::FailTestIf(timeoutHelper.IsExpired,
                "Verification failed with a Timeout of {0} seconds. Check logs for details.",
                timeout.TotalSeconds());                             
        }
        
        Sleep(interval);
        interval = min<DWORD>(interval + 500, 3000);
    }

    PrintAllQuorumAndRebuildLostFailoverUnits();
    testFederation_.CheckDeletedNodes();
    TestSession::WriteInfo(TraceSource, "All verification succeeded.");

    return true;
}

bool FabricTestDispatcher::VerifyAll(bool verifyPLB)
{
    ClearNodeServicePlacementList();

    if (!VerifyFMM())
    {
        return false;
    }

    auto fmm = GetFMM();
    if (!fmm)
    {
        TestSession::WriteInfo(TraceSource, "FMM not valid.");
        return false;
    }

    size_t fmmUpNodeCount = 0;
    auto fmmNodes = fmm->SnapshotNodeCache(fmmUpNodeCount);
    if (!VerifyNodes(fmmNodes, fmmUpNodeCount, L"FMM"))
    {
        return false;
    }

    auto fm = VerifyFM();
    if (!fm)
    {
        return false;
    }

    size_t fmUpNodeCount = 0;
    auto fmNodes = fm->SnapshotNodeCache(fmUpNodeCount);
    if (!VerifyNodes(fmNodes, fmUpNodeCount, L"FM"))
    {
        return false;
    }

    map<FailoverUnitId, FailoverUnitSnapshot> failoverUnits;
    auto ftList = fm->SnapshotGfum();
    for(auto it = ftList.begin(); it != ftList.end(); ++it)
    {
        failoverUnits.insert(make_pair(it->Id, *it));
    }

    auto services = fm->SnapshotServiceCache();

    bool result = VerifyGFUM(failoverUnits, services);
    UpdateQuorumOrRebuildLostFailoverUnits();
    if (!result)
    {
        return false;
    }

    if (!VerifyLFUM())
    {
        return false;
    }

    if (!VerifyServicePlacement())
    {
        return false;
    }

    if (verifyPLB)
    {
        Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * plb = static_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing*>(&fm->PLB);
        TestSession::FailTestIf(!plb, "PLB not valid");

        if (!plb->Test_IsPLBStable())
        {
            TestSession::WriteInfo(TraceSource, "PLB not stable");
            return false;
        }
    }

    if(!FabricTestSessionConfig::GetConfig().AllowQuoumLostFailoverUnitsOnVerify && IsQuorumLostForAnyFailoverUnit())
    {
        PrintAllQuorumAndRebuildLostFailoverUnits();
        return false;
    }

    if (FabricTestSessionConfig::GetConfig().HealthVerificationEnabled && Management::ManagementConfig::GetConfig().TargetReplicaSetSize > 0)
    {
        HealthManagerReplicaSPtr hm = GetHM();
        if (!hm)
        {
            TestSession::WriteInfo(TraceSource, "HM not ready");
            return false;
        }

        result = fabricClientHealth_->VerifyNodeHealthReports(hm, fmNodes, fmUpNodeCount) &&
            fabricClientHealth_->VerifyParititionHealthReports(hm, failoverUnits) &&
            fabricClientHealth_->VerifyReplicaHealthReports(hm, failoverUnits) &&
            fabricClientHealth_->VerifyServiceHealthReports(hm, services) &&
            fabricClientHealth_->VerifyApplicationHealthReports(hm, services);

        if (result)
        {
            map<DeployedServicePackageHealthId, FailoverUnitId> packages;
            for (FabricTestNodeSPtr const & testNode : testFederation_.GetNodes())
            {
                testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
                {
                    auto ra = reliabilityTestHelper.GetRA();
                    auto lfum = ra->SnapshotLfum();
                    for(auto lfumVisitor = lfum.begin(); lfumVisitor != lfum.end(); ++lfumVisitor)
                    {
                        auto & rafuPtr = *lfumVisitor;
                        wstring const & appName = rafuPtr.ServiceDescription.ApplicationName;
                        auto localReplica = rafuPtr.GetLocalReplica();
                        if (appName.size() > 0 && localReplica != nullptr && localReplica->IsUp && rafuPtr.ServiceTypeRegistration != nullptr)
                        {
                            auto const & packageName = rafuPtr.ServiceDescription.Type.ServicePackageId.ServicePackageName;
                            auto servicePackageActivationId = rafuPtr.ServiceTypeRegistration->ServicePackagePublicActivationId;

                            packages[DeployedServicePackageHealthId(appName, packageName, servicePackageActivationId, localReplica->FederationNodeId.IdValue)] = rafuPtr.FailoverUnitDescription.FailoverUnitId;
                        }
                    }
                });
            }

            result = fabricClientHealth_->VerifyDeployedServicePackageAndApplicationHealthReports(hm, move(packages));
        }

        return result;
    }

    return true;
}

bool FabricTestDispatcher::VerifyServicePlacement()
{
    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        if (!node->VerifyAllServices())
        {
            return false;
        }
    }

    return true;
}

void FabricTestDispatcher::ClearNodeServicePlacementList()
{
    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        node->ClearServicePlacementList();
    }
}

bool FabricTestDispatcher::VerifyLFUM()
{
    if (testFederation_.Count == 0)
    {
        TestSession::WriteInfo(TraceSource, "No nodes");
        return true;
    }

    auto fm = GetFM();
    FailoverManagerTestHelperUPtr fmm;
    if(FailoverConfig::GetConfig().TargetReplicaSetSize > 0)
    {
        fmm = GetFMM();
    }

    for (FabricTestNodeSPtr const & testNode : testFederation_.GetNodes())
    {
        bool result = false;

        Common::StopwatchTime nodeOpenTimestamp = testNode->Node->NodeOpenTimestamp;
        testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
        {
            auto ra = reliabilityTestHelper.GetRA();
            result = VerifyLFUMOnNode(*ra, reliabilityTestHelper.Reliability.FederationWrapper.Id, fm, fmm, nodeOpenTimestamp);
        });

        if (!result)
        {
            return result;
        }
    }

    return true;
}

bool FabricTestDispatcher::VerifyNodeActivationStatus(
    ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper & ra,
    bool isFmm,
    NodeSnapshot const & fmNodeInfo)
{
    bool raIsActivated; int64 raSequenceNumber;

    ra.GetNodeActivationStatus(isFmm, raIsActivated, raSequenceNumber);

    // If the FM intent is anything other than Restart then on the RA the node should be activated
    // The sequence numbers match only if intent is Restart or None
    if (fmNodeInfo.DeactivationInfo.Intent != NodeDeactivationIntent::None && !fmNodeInfo.DeactivationInfo.IsRestart)
    {
        if (!raIsActivated)
        {
            TestSession::WriteInfo(TraceSource, "Deactivation state do not match for node {0} (isFMM = {1}). RA {2}. FM {3}", ra.NodeId, isFmm, raIsActivated, fmNodeInfo.DeactivationInfo.Intent);
            return false;
        }

        return true;
    }

    bool fmIsActivated = fmNodeInfo.DeactivationInfo.Intent == NodeDeactivationIntent::None;
    if (fmIsActivated != raIsActivated || fmNodeInfo.DeactivationInfo.SequenceNumber != raSequenceNumber)
    {
        TestSession::WriteInfo(
            TraceSource,
            "Deactivation state do not match for node {0} (isFMM = {1}). RA Is Node Activated {2}. FM Deactivation Intent {3}. RA Sequence Number {4}. FM Sequence Number {5}",
            ra.NodeId,
            isFmm,
            raIsActivated,
            fmNodeInfo.DeactivationInfo.Intent,
            raSequenceNumber,
            fmNodeInfo.DeactivationInfo.SequenceNumber);
        return false;
    }

    return true;
}

class LastReplicaUpVerifier
{
    DENY_COPY(LastReplicaUpVerifier);

public:
    LastReplicaUpVerifier(StopwatchTime nodeOpenTimestamp, NodeId nodeId)
    : nodeOpenTimestamp_(nodeOpenTimestamp),
      nodeId_(nodeId)
    {
    }

    void VerifyRA(bool isLastReplicaUpAcknowledged, wstring const & tag)
    {
        if (!ShouldLastReplicaUpBeComplete())
        {
            return;
        }

        TestSession::FailTestIf(!isLastReplicaUpAcknowledged, "On RA at node {0} Last replica up for {1} not acknowledged after {2}", nodeId_, tag, GetTimeSinceNodeOpen());
    }

    void VerifyFM(NodeSnapshot const & nodeSnapshot, wstring const & tag)
    {
        if (!ShouldLastReplicaUpBeComplete())
        {
            return;
        }

        TestSession::FailTestIf(!nodeSnapshot.IsReplicaUploaded, "On FM at node {0} at {1} Last Replica Up not acknowledged after {2}", nodeId_, tag, GetTimeSinceNodeOpen());
    }

private:
    Common::TimeSpan GetTimeSinceNodeOpen() const
    {
        return Stopwatch::Now() - nodeOpenTimestamp_;
    }

    bool ShouldLastReplicaUpBeComplete()
    {
        // Wait for 2 minute more than the reopen wait duration
        return GetTimeSinceNodeOpen() > FailoverConfig::GetConfig().ReopenSuccessWaitInterval + FabricTestSessionConfig::GetConfig().LastReplicaUpTimeout;
    }

    NodeId const nodeId_;
    StopwatchTime const nodeOpenTimestamp_;
};

// Verify FM and RA FT
class FTVerifier
{
    DENY_COPY(FTVerifier);

public:
    FTVerifier(
        bool isFmm,
        FailoverUnitSnapshot const * fmfu,
        ReconfigurationAgentComponentTestApi::FailoverUnitSnapshot const * rafu,
        Federation::NodeId const & nodeId)
    : fmfu_(fmfu),
      rafu_(rafu),
      isFmm_(isFmm),
      nodeId_(nodeId)
    {
    }

    bool VerifyReplicaDoesNotExistOnLfum()
    {
        if (!rafu_ || rafu_->State == ReconfigurationAgentComponent::FailoverUnitStates::Closed)
        {
            return true;
        }

        auto localReplica = rafu_->GetLocalReplica();
        if (rafu_->LocalReplicaRole != Reliability::ReplicaRole::None &&
            localReplica->State != ReconfigurationAgentComponent::ReplicaStates::StandBy)
        {
            TestSession::WriteInfo(TraceSource, "Replica exists in LFUM but not GFUM: RA side {0}\r\nFM side {1}",
                *rafu_,
                *fmfu_,
                nodeId_);
            return false;
        }

        return true;
    }

    bool VerifyReplicaExistsOnLfum(ReplicaSnapshot const & fmReplica)
    {
        if (!rafu_ || rafu_->State == Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Closed)
        {
            if (!fmReplica.IsDropped && !(fmReplica.IsInBuild && !fmReplica.IsUp))
            {
                TestSession::WriteInfo(TraceSource, "FailoverUnit {0} exists at FM but not on LFUM for nodeId {1}", fmfu_->Id, nodeId_);
                return false;
            }
        }

        return true;
    }

    bool VerifyReplicaPackageVersionMatches(ReplicaSnapshot const & fmReplica)
    {
        bool isMismatch = fmReplica.IsUp && fmReplica.Version != rafu_->GetLocalReplica()->PackageVersionInstance;
        if (!isMismatch)
        {
            return true;
        }


        /*
            Handle the case where the replica is a new replica being built on a node
            In this case the package version instance on the FM must be 0 and on the RA the deactivation info is Dropped
        */
        if (fmReplica.Version == ServicePackageVersionInstance::Invalid &&
            rafu_->IsDeactivationInfoDropped)
        {
            return true;
        }

        TestSession::WriteInfo(
            TraceSource,
            "Replica version does not match. At GFUM {0}, at LFUM {1}, for FT {2}/{3}",
            fmReplica.Version,
            rafu_->GetLocalReplica()->PackageVersionInstance,
            *fmfu_,
            *rafu_);
        return false;
    }

    bool VerifyServiceInfoNotStale()
    {
        auto service = fmfu_->GetServiceInfo();
        if (service.IsStale)
        {
            TestSession::WriteInfo(
                TraceSource,
                "Waiting for ServiceInfo at FM to get updated: {0}",
                service);
            return false;
        }

        return true;
    }

    bool VerifyServiceDescriptionStatefulness()
    {
        if (rafu_->ServiceDescription.IsStateful != fmfu_->IsStateful)
        {
            TestSession::FailTest(
                "At GFUM IsStateful({0}) does not match RA IsStateful({1}) for FailoverUnit {2}/{3}",
                fmfu_->IsStateful,
                rafu_->ServiceDescription.IsStateful,
                *fmfu_, rafu_->ServiceDescription);
        }

        return true;
    }

    bool VerifyServiceDescriptionUpdateVersion()
    {
        auto fmService = fmfu_->GetServiceInfo();
        if (rafu_->ServiceDescription.UpdateVersion != fmService.ServiceDescription.UpdateVersion)
        {
            TestSession::WriteInfo(
                TraceSource,
                "Update version does not match. At GFUM {0}. At LFUM {1}. for FT {2}/{3}",
                fmService.ServiceDescription.UpdateVersion,
                rafu_->ServiceDescription.UpdateVersion,
                *fmfu_,
                *rafu_);
            return false;
        }

        return true;
    }

    bool VerifyCCRole(ReplicaSnapshot const & fmReplica)
    {
        //Verify that configuration role for replica in RA matches that if replica in FM
        if ((fmfu_->IsStateful && rafu_->LocalReplicaRole != fmReplica.CurrentConfigurationRole)
            || (!fmfu_->IsStateful &&
            (rafu_->LocalReplicaRole != Reliability::ReplicaRole::None ||
            fmReplica.CurrentConfigurationRole != Reliability::ReplicaRole::None)))
        {

            TestSession::WriteInfo(TraceSource, "Replica role in LFUM {0} does not match role in GFUM {1} for IsStateful = {2} at nodeId {3} for FUID {4}",
                rafu_->LocalReplicaRole,
                fmReplica.CurrentConfigurationRole,
                fmfu_->IsStateful,
                nodeId_,
                fmfu_->Id);
            return false;
        }

        return true;
    }

    bool VerifyServiceTypeIdentifiers()
    {
        if (!rafu_->LocalReplicaOpen)
        {
            return true;
        }

        auto serviceInfo = fmfu_->GetServiceInfo();
        auto serviceTypeInfo = serviceInfo.GetServiceType();
        if(serviceTypeInfo.Type != rafu_->ServiceTypeRegistration->ServiceTypeId)
        {
            TestSession::WriteInfo(TraceSource,
                "ServiceTypeIdentifiers dont match for FailoverUnit {0}. FM side {1}, RA side {2} for node {3}",
                fmfu_->Id,
                serviceTypeInfo.Type,
                rafu_->ServiceTypeRegistration->ServiceTypeId,
                nodeId_);
            return false;
        }

        return true;
    }

    bool VerifyServicePackageVersionInstance()
    {
        if (!rafu_->LocalReplicaOpen)
        {
            return true;
        }

        auto service = fmfu_->GetServiceInfo();
        auto localReplica = rafu_->GetLocalReplica();
        if(service.ServiceDescription.PackageVersion != localReplica->PackageVersionInstance.Version)
        {
            TestSession::WriteInfo(TraceSource,
                "ServicePackageVersion dont match for FailoverUnit {0}. FM side {1}, RA side {2} for node {3}",
                fmfu_->Id,
                service.ServiceDescription.PackageVersion,
                localReplica->PackageVersionInstance,
                nodeId_);
            return false;
        }

        return true;
    }

    bool VerifyOfflineness(ReplicaSnapshot const & fmReplica)
    {
        auto localReplica = rafu_->GetLocalReplica();
        if(fmReplica.IsOffline != localReplica->IsOffline)
        {
            TestSession::WriteInfo(TraceSource,
                "Replica states dont match for FailoverUnit {0}. FM side {1}, RA side {2}",
                fmfu_->Id,
                fmReplica, *rafu_);
            return false;
        }

        return true;
    }

    bool VerifyStandbyness(ReplicaSnapshot const & fmReplica)
    {
        auto localReplica = rafu_->GetLocalReplica();
        if(!fmReplica.IsOffline && fmReplica.IsStandBy != localReplica->IsStandBy)
        {
            TestSession::WriteInfo(TraceSource,
                "Replica states dont match for up replica in FailoverUnit {0}. FMM side {1}, RA side {2}",
                fmfu_->Id,
                fmReplica, *rafu_);
            return false;
        }

        return true;
    }

    bool VerifyEpoch()
    {
        //For Stateful services verify that the configuration verions in RA and FM match
        if (rafu_->LocalReplicaRole >= Reliability::ReplicaRole::Secondary &&
            rafu_->CurrentConfigurationEpoch != fmfu_->CurrentConfigurationEpoch)
        {
            TestSession::WriteInfo(TraceSource, "Configuration epoch in LFUM {0} does not match epoch in GFUM {1} for partition {2} on node {3}",
                rafu_->CurrentConfigurationEpoch,
                fmfu_->CurrentConfigurationEpoch,
                fmfu_->Id,
                nodeId_);
            return false;
        }

        return true;
    }

private:
    bool const isFmm_;
    Federation::NodeId const nodeId_;
    FailoverUnitSnapshot const * fmfu_;
    ReconfigurationAgentComponentTestApi::FailoverUnitSnapshot const * rafu_;
};

bool FabricTestDispatcher::VerifyLFUMOnNode(
    ReconfigurationAgentComponentTestApi::ReconfigurationAgentTestHelper & ra,
    NodeId nodeId,
    FailoverManagerTestHelperUPtr & fm,
    FailoverManagerTestHelperUPtr & fmm,
    Common::StopwatchTime nodeOpenTimestamp)
{
    TestSession::FailTestIf(!ra.IsOpen, "RA is not Open at {0}", nodeId);
    TestSession::FailTestIf(!fm, "FM not ready");

    LastReplicaUpVerifier lastReplicaUpVerifier(nodeOpenTimestamp, nodeId);
    lastReplicaUpVerifier.VerifyRA(ra.IsLastReplicaUpAcknowledged(false), L"FM");
    lastReplicaUpVerifier.VerifyRA(ra.IsLastReplicaUpAcknowledged(true), L"Fmm");

    map<FailoverUnitId, ReconfigurationAgentComponentTestApi::FailoverUnitSnapshot> lfum;

    {
        auto lfumAsList = ra.SnapshotLfum();
        for(auto it = lfumAsList.begin(); it != lfumAsList.end(); ++it)
        {
            lfum.insert(make_pair(it->FailoverUnitDescription.FailoverUnitId, *it));
        }
    }

    // Verify FM and RA node activation status
    {
        auto fmNode = fm->FindNodeByNodeId(nodeId);
        TestSession::FailTestIf(!fmNode, "NodeInfo {0} not found at FM", nodeId);
        VerifyNodeActivationStatus(ra, false, *fmNode);
        lastReplicaUpVerifier.VerifyFM(*fmNode, L"FM");

        auto fmmNode = fmm->FindNodeByNodeId(nodeId);
        TestSession::FailTestIf(!fmmNode, "NodeInfo {0} not found at FMM", nodeId);
        VerifyNodeActivationStatus(ra, true, *fmmNode);
        lastReplicaUpVerifier.VerifyFM(*fmmNode, L"Fmm");
    }

    auto gfum = fm->SnapshotGfum();
    for(auto fmfu = gfum.begin(); fmfu != gfum.end(); ++fmfu)
    {
        auto rafuIter = lfum.find(fmfu->Id);
        ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr rafu;
        if (rafuIter  != lfum.end())
        {
            rafu = make_unique<ReconfigurationAgentComponentTestApi::FailoverUnitSnapshot>(rafuIter->second);
        }

        auto fmReplica = fmfu->GetReplica(nodeId);

        FTVerifier ftVerifier(false, &(*fmfu), rafu.get(), nodeId);

        if (fmfu->IsQuorumLost())
        {
            continue;
        }

        if (fmfu->IsChangingConfiguration)
        {
            bool isQuorumLost = false;
            if(fmfu->ReconfigurationPrimary->FederationNodeId == nodeId)
            {
                isQuorumLost = IsQuorumLostForReconfigurationFU(rafu);
            }
            else
            {
                isQuorumLost = IsQuorumLostForReconfigurationFU(*fmfu);
            }

            if(isQuorumLost)
            {
                // Quorum lost for this FailoverUnit so continue and skip validation
                continue;
            }
        }

        if (fmReplica != nullptr)
        {
            if (!rafu || rafu->State == Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Closed)
            {
                if (!ftVerifier.VerifyReplicaExistsOnLfum(*fmReplica))
                {
                    return false;
                }
            }
            else if (!ftVerifier.VerifyReplicaPackageVersionMatches(*fmReplica))
            {
                return false;
            }

            // Skip validation if the replica is in standby mode or offline
            else if (!fmReplica->IsStandBy && !fmReplica->IsOffline)
            {
                if (!ftVerifier.VerifyServiceDescriptionStatefulness())
                {
                    return false;
                }

                if (!ftVerifier.VerifyServiceInfoNotStale())
                {
                    return false;
                }

                if (!ftVerifier.VerifyServiceDescriptionUpdateVersion())
                {
                    return false;
                }

                if (!ftVerifier.VerifyCCRole(*fmReplica))
                {
                    return false;
                }

                if (!ftVerifier.VerifyServiceTypeIdentifiers())
                {
                    return false;
                }

                if (!ftVerifier.VerifyServicePackageVersionInstance())
                {
                    return false;
                }

                if (!ftVerifier.VerifyEpoch())
                {
                    return false;
                }
            }
            else
            {
                if (fmReplica->IsDropped)
                {
                    TestSession::WriteInfo(TraceSource,
                        "Replica is dropped on FM but not on RA. FM side {1}, RA side {2}",
                        fmfu->Id,
                        *fmReplica, *rafu);
                    return false;
                }

                if(!ftVerifier.VerifyOfflineness(*fmReplica))
                {
                    return false;
                }

                if(!ftVerifier.VerifyStandbyness(*fmReplica))
                {
                    return false;
                }

                if (!ftVerifier.VerifyServiceTypeIdentifiers())
                {
                    return false;
                }

                if (!ftVerifier.VerifyServicePackageVersionInstance())
                {
                    return false;
                }
            }
        }
        else if (fmReplica == nullptr)
        {
            if (!ftVerifier.VerifyReplicaDoesNotExistOnLfum())
            {
                return false;
            }
        }
    }

    if (FailoverConfig::GetConfig().TargetReplicaSetSize > 0)
    {
        TestSession::FailTestIf(!fmm, "FMM not ready");
        auto fmm_gfum = fmm->SnapshotGfum();
        for(auto fmmfu = fmm_gfum.begin(); fmmfu != fmm_gfum.end(); ++fmmfu)
        {
            auto rafu = ra.FindFTByPartitionId(fmmfu->Id);
            auto fmmReplica = fmmfu->GetReplica(nodeId);

            if (fmmfu->IsQuorumLost())
            {
                continue;
            }

            FTVerifier ftVerifier(false, &(*fmmfu), rafu.get(), nodeId);

            if (fmmReplica != nullptr)
            {
                if (!rafu || rafu->State == Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Closed)
                {
                    if (!ftVerifier.VerifyReplicaExistsOnLfum(*fmmReplica))
                    {
                        return false;
                    }
                }

                // Skip validation if the replica is in standby mode or offline
                else if (!fmmReplica->IsStandBy && !fmmReplica->IsOffline)
                {
                    if (!ftVerifier.VerifyServiceDescriptionStatefulness())
                    {
                        return false;
                    }

                    if (!ftVerifier.VerifyCCRole(*fmmReplica))
                    {
                        return false;
                    }

                    if (!ftVerifier.VerifyEpoch())
                    {
                        return false;
                    }
                }
                else
                {
                    if(!ftVerifier.VerifyOfflineness(*fmmReplica))
                    {
                        return false;
                    }

                    if(!ftVerifier.VerifyStandbyness(*fmmReplica))
                    {
                        return false;
                    }
                }
            }
            else if (fmmReplica == nullptr)
            {
                if (!ftVerifier.VerifyReplicaDoesNotExistOnLfum())
                {
                    return false;
                }
            }

        }
    }

    for(auto rafuIter = lfum.begin(); rafuIter != lfum.end(); ++rafuIter)
    {
        auto rafuPtr = &(rafuIter->second);
        if (rafuPtr->State == ReconfigurationAgentComponent::FailoverUnitStates::Closed)
        {
            const int deletedFTDelayFactor = 4;
            const int droppedFTDelayFactor = 2;

            auto maxTimeToHoldDeletedFT = TimeSpan::FromSeconds(static_cast<double>(FailoverConfig::GetConfig().DeletedFailoverUnitTombstoneDuration.TotalSeconds() * deletedFTDelayFactor));
            auto maxTimeToHoldDroppedFT = TimeSpan::FromSeconds(static_cast<double>(FailoverConfig::GetConfig().DroppedFailoverUnitTombstoneDuration.TotalSeconds() * droppedFTDelayFactor));

            auto deltaTime = DateTime::Now() - rafuPtr->LastUpdatedTime;

            if (rafuPtr->LocalReplicaDeleted)
            {
                if (deltaTime >= maxTimeToHoldDeletedFT)
                {
                    TestSession::FailTest("Replica on RA {0} on node {1} should not exist because it has been deleted for too long and should have been removed", *rafuPtr, nodeId);
                }
            }
            else
            {
                if (deltaTime >= maxTimeToHoldDroppedFT)
                {
                    TestSession::FailTest("Replica on RA {0} on node {1} should not exist because it has been dropped for too long and should have been removed", *rafuPtr, nodeId);
                }
            }
        }
    }

    return true;
}

bool FabricTestDispatcher::IsQuorumLostForReconfigurationFU(ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr & primaryRafu)
{
    if(primaryRafu && primaryRafu->ReconfigurationStage == ReconfigurationAgentComponent::FailoverUnitReconfigurationStage::Phase1_GetLSN)
    {
        size_t ccInvalidLSNCount = 0;
        size_t pcInvalidLSNCount = 0;
        size_t ccCount = 0;
        size_t pcCount = 0;
        size_t ccUpReplicaCount = 0;
        size_t pcUpReplicaCount = 0;
        auto replicas = primaryRafu->GetReplicas();
        for (auto replicaIter = replicas.begin(); replicaIter != replicas.end(); replicaIter++)
        {
            auto const& replica = *replicaIter;

            if(replica.IsInCurrentConfiguration)
            {
                ++ccCount;

                if(replica.MessageStage == ReconfigurationAgentComponent::ReplicaMessageStage::None &&
                    replica.ReplicaDeactivationInfo.IsDropped &&
                    replica.IsUp)
                {
                    ++ccInvalidLSNCount;
                }

                if(replica.IsUp)
                {
                    ++ccUpReplicaCount;
                }
            }

            if(replica.IsInPreviousConfiguration)
            {
                ++pcCount;

                if(replica.MessageStage == ReconfigurationAgentComponent::ReplicaMessageStage::None &&
                    replica.ReplicaDeactivationInfo.IsDropped &&
                    replica.IsUp)
                {
                    ++pcInvalidLSNCount;
                }

                if(replica.IsUp)
                {
                    ++pcUpReplicaCount;
                }
            }
        }

        size_t ccReadQuorumSize = (ccCount + 1) / 2;
        size_t pcReadQuorumSize = (pcCount + 1) / 2;
        if(ccReadQuorumSize > (ccUpReplicaCount - ccInvalidLSNCount))
        {
            return true;
        }

        if(pcReadQuorumSize > (pcUpReplicaCount - pcInvalidLSNCount))
        {
            return true;
        }
    }

    return false;
}

bool FabricTestDispatcher::EnableLogTruncationTimestampValidation(Common::StringCollection const & params)
{
    if (params.size() != 2)
    {
        return false;
    }

    wstring serviceName(params[0]);
    wstring action(params[1]);

    if (action == L"start")
    {
        checkpointTruncationTimestampValidator_->AddService(serviceName);
    }
    else if (action == L"stop")
    {
        checkpointTruncationTimestampValidator_->RemoveService(serviceName);
    }
    else 
    {
        TestSession::FailTest("Invalid action. Expected enablelogtruncationtimestampvalidation <serviceName> <start/stop> ");
    }
    return true;
}

bool FabricTestDispatcher::WaitForAllToApplyLsn(Common::StringCollection const & params)
{
    if (params.size() < 3)
    {
        if (params.size() != 1)
        {
            FABRICSESSION.PrintHelp(FabricTestCommands::WaitForAllToApplyLsn);
            return false;
        }
    }

    wstring serviceName(params[0]);
    NodeId nodeId;
    TimeSpan timeout = params.size() > 3 ? TimeSpan::FromSeconds(Double_Parse(params[3])) : TimeSpan::FromSeconds(100);

    if (params.size() == 1)
    {
        StringCollection findArgs;
        std::wstring arg1 = L"Primary";
        findArgs.push_back(arg1);
        findArgs.push_back(serviceName);
        std::wstring fuPrimaryLocation = GetFailoverUnitLocationState(findArgs);

        TestSession::WriteInfo(
            TraceSource,
            "{0} {1} node = {2}",
            serviceName,
            arg1,
            fuPrimaryLocation);

        nodeId = ParseNodeId(fuPrimaryLocation);
        return WaitForAllToApplyLsnExtension(serviceName, nodeId, timeout);
    }

    nodeId = ParseNodeId(params[1]);
    FABRIC_SEQUENCE_NUMBER targetLsn = stoll(params[2]);
    WaitForTargetLsn(serviceName, nodeId, timeout, targetLsn);
    return false;
}

bool FabricTestDispatcher::WaitForAllToApplyLsnExtension(std::wstring & serviceName, NodeId & nodeId, TimeSpan timeout)
{
    ErrorCode er;
    ITestStoreServiceSPtr testStoreService;

    TestSession::FailTestIfNot(
        FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName, nodeId, testStoreService),
        "Failed to get Service:{0} on Node:{1}", serviceName, nodeId);

    FABRIC_SEQUENCE_NUMBER targetLsn = CalculateTargetLsn(serviceName, nodeId);

    return WaitForTargetLsn(serviceName, nodeId, timeout, targetLsn);
}

FABRIC_SEQUENCE_NUMBER FabricTestDispatcher::CalculateTargetLsn(std::wstring & serviceName, NodeId & nodeId)
{
    ErrorCode er;
    ScopedHeap heap;
    ITestStoreServiceSPtr service;

    TestSession::FailTestIfNot(
        FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName, nodeId, service),
        "Failed to get Service:{0} on Node:{1}", serviceName, nodeId);

    ServiceModel::ReplicatorStatusQueryResultSPtr queryResultSPtr;
    std::shared_ptr<ServiceModel::PrimaryReplicatorStatusQueryResult> primaryQueryResultSPtr;

    FABRIC_REPLICATOR_STATUS_QUERY_RESULT queryResult = {};
    FABRIC_SEQUENCE_NUMBER targetLsn = -1;
    int repeatLastLsnCount = 0;

    DateTime deadline = DateTime::Now() + TimeSpan::FromSeconds(2);

    while (DateTime::Now() < deadline)
    {
        Sleep(100);
        er = service->GetReplicatorQueryResult(queryResultSPtr);

        if (!er.IsSuccess())
        {
            TestSession::FailTest(
                "ITestStoreServiceSPtr::GetReplicatorQueryResult failed with {0}",
                er.ErrorCodeValueToString());
        }

        primaryQueryResultSPtr =
            static_pointer_cast<ServiceModel::PrimaryReplicatorStatusQueryResult>(queryResultSPtr);

        er = primaryQueryResultSPtr->ToPublicApi(heap, queryResult);

        if (!er.IsSuccess())
        {
            TestSession::FailTest(
                "ITestStoreServiceSPtr::GetReplicatorQueryResult failed with {0}",
                er.ErrorCodeValueToString());
        }

        auto primaryReplicatorStatus = static_cast<FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT *>(queryResult.Value);
        FABRIC_SEQUENCE_NUMBER newLastLsn = primaryReplicatorStatus->ReplicationQueueStatus->LastSequenceNumber;

        if (newLastLsn > targetLsn)
        {
            if (targetLsn > 0)
            {
                TestSession::WriteInfo(
                    TraceSource,
                    "{0} targetLsn updated : {1} -> {2}",
                    serviceName,
                    targetLsn,
                    newLastLsn);
            }

            targetLsn = newLastLsn;
        }
        else if (newLastLsn == targetLsn)
        {
            repeatLastLsnCount++;
        }

        // Exit if the same last LSN is seen over 3 loops
        if (repeatLastLsnCount == 3)
        {
            break;
        }
    }

    TestSession::FailTestIfNot(
        targetLsn > 0,
        "Failed to retrieve targetLsn for {0} on node {1}",
        serviceName,
        nodeId);

    return targetLsn;
}

bool FabricTestDispatcher::WaitForTargetLsn(std::wstring & serviceName, NodeId & nodeId, TimeSpan timeout, FABRIC_SEQUENCE_NUMBER targetLsn)
{
    ITestStoreServiceSPtr service;

    TestSession::FailTestIfNot(
        FABRICSESSION.FabricDispatcher.Federation.TryFindStoreService(serviceName, nodeId, service),
        "Failed to get Service:{0} on Node:{1}", serviceName, nodeId);

    FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS counters;
    ErrorCode er;
    DateTime deadline = DateTime::Now() + timeout;

    bool reachedTargetApplyAckLsn = false;

    while (!reachedTargetApplyAckLsn)
    {
        er = service->GetReplicationQueueCounters(counters);
        if (!er.IsSuccess())
        {
            TestSession::FailTest(
                "ITestStoreServiceSPtr::GetReplicationQueueCounters failed with {0}",
                er.ErrorCodeValueToString());
        }

        TestSession::WriteInfo(
            TraceSource,
            "{0}.{1} allApplyAckLsn: {1}",
            serviceName,
            nodeId,
            counters.allApplyAckLsn);

        if (counters.allApplyAckLsn >= targetLsn)
        {
            return true;
        }

        if (deadline > DateTime::Now())
        {
            TestSession::WriteInfo(
                TraceSource,
                "{0}.{1} target lsn: {2} / current lsn: {3}",
                serviceName,
                nodeId,
                targetLsn,
                counters.allApplyAckLsn);

            Sleep(500);
        }
        else
        {
            TestSession::FailTest(
                "{0}.{1} target lsn: {2} / current lsn: {3}",
                serviceName,
                nodeId,
                targetLsn,
                counters.allApplyAckLsn);
        }
    }

    return false;
}

bool FabricTestDispatcher::CheckContainers(Common::StringCollection const &)
{
    auto & dockerProcessMgr = fabricActivationManager_->ProcessActivationManagerObj->ContainerActivatorObj->DockerProcessManagerObj;

    if (HostingConfig::GetConfig().DisableContainers ||
        !dockerProcessMgr.IsDockerServicePresent)
    {
        FabricTestSession::GetFabricTestSession().FinishExecution(1);
    }

    return true;
}

bool FabricTestDispatcher::TryGetPrimaryRAFU(
    FailoverUnitSnapshot const& fmfu,
    ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr & primaryRafu)
{
    for (FabricTestNodeSPtr const & testNode : testFederation_.GetNodes())
    {
        if(fmfu.ReconfigurationPrimary->FederationNodeId == testNode->Id)
        {
            testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
            {
                auto ra = reliabilityTestHelper.GetRA() ;
                primaryRafu = ra->FindFTByPartitionId(fmfu.Id);
            });

            break;
        }
    }

    if (primaryRafu && primaryRafu->State == ReconfigurationAgentComponent::FailoverUnitStates::Closed)
    {
        // Ignore closed and null FT on the node
        primaryRafu = nullptr;
    }

    return primaryRafu != nullptr;;
}

bool CalculateIfFMConfigurationHasReadQuorum(std::vector<ReplicaSnapshot> const & replicas, function<bool(ReplicaSnapshot const&)> isInConfigurationFunc)
{
    size_t replicaSetSize = 0;
    size_t availableReplicas = 0;

    for (auto const & it : replicas)
    {
        if (!isInConfigurationFunc(it))
        {
            continue;
        }

        replicaSetSize++;

        if (it.IsUp && !it.IsCreating)
        {
            availableReplicas++;
        }
    }

    size_t quorumSize = (replicaSetSize + 1) / 2;
    return availableReplicas >= quorumSize;
}

bool CalculateIfFMConfigurationHasReadQuorum(std::vector<ReplicaSnapshot> const & replicas)
{
    auto pcHasReadQuorum = CalculateIfFMConfigurationHasReadQuorum(replicas, [](ReplicaSnapshot const & r) { return r.IsInPreviousConfiguration; });
    auto ccHasReadQuorum = CalculateIfFMConfigurationHasReadQuorum(replicas, [](ReplicaSnapshot const & r) { return r.IsInCurrentConfiguration; });

    return pcHasReadQuorum && ccHasReadQuorum;
}

bool FabricTestDispatcher::IsQuorumLostForReconfigurationFU(FailoverUnitSnapshot const& fmfu)
{
    ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr rafu;

    if(TryGetPrimaryRAFU(fmfu, rafu))
    {
        return IsQuorumLostForReconfigurationFU(rafu);
    }

    /*
        Calculate based on the FM FT if there is quorum or not
        Need to ignore the replica which is still creating
    */

    return !CalculateIfFMConfigurationHasReadQuorum(fmfu.GetReplicas());
}

void FabricTestDispatcher::UpdateQuorumOrRebuildLostFailoverUnits()
{
    auto fm = GetFM();
    if(!fm)
    {
        TestSession::WriteWarning(TraceSource, "FM not ready hence returning from UpdateQuorumLostFailoverUnits");
        return;
    }

    AcquireWriteLock grab(quorumAndRebuildLostFailoverUnitsLock_);
    auto gfum = fm->SnapshotGfum();
    vector<Guid> fmFUs;
    quorumLostFailoverUnits_.clear();
    rebuildLostFailoverUnits_.clear();
    toBeDeletedFailoverUnits_.clear();
    for(auto failoverUnit = gfum.begin(); failoverUnit != gfum.end(); ++failoverUnit)
    {
        fmFUs.push_back(failoverUnit->Id.Guid);
        if(failoverUnit->IsToBeDeleted)
        {
            toBeDeletedFailoverUnits_.push_back(failoverUnit->Id.Guid);
        }
        else
        {
            if(failoverUnit->IsQuorumLost())
            {
                quorumLostFailoverUnits_.push_back(failoverUnit->Id.Guid);
            }
            else if(failoverUnit->IsChangingConfiguration)
            {
                bool isQuorumLost = IsQuorumLostForReconfigurationFU(*failoverUnit);
                if(isQuorumLost)
                {
                    quorumLostFailoverUnits_.push_back(failoverUnit->Id.Guid);
                }
            }
        }
    }

    if(FabricTestSessionConfig::GetConfig().AllowServiceAndFULossOnRebuild)
    {
        vector<Guid> testFUs;
        ServiceMap.GetAllFUs(testFUs);
        for(auto iter = testFUs.begin(); iter != testFUs.end(); iter++)
        {
            auto fmFUIter = find(fmFUs.begin(), fmFUs.end(), *iter);
            if(fmFUIter == fmFUs.end())
            {
                rebuildLostFailoverUnits_.push_back(*iter);
            }
        }
    }
}

bool FabricTestDispatcher::IsQuorumLostForAnyFailoverUnit()
{
    AcquireReadLock grab(quorumAndRebuildLostFailoverUnitsLock_);
    return quorumLostFailoverUnits_.size() > 0;
}

void FabricTestDispatcher::PrintAllQuorumAndRebuildLostFailoverUnits()
{
    AcquireReadLock grab(quorumAndRebuildLostFailoverUnitsLock_);
    wstring quorumLostTextResult;
    StringWriter quorumLostWriter(quorumLostTextResult);
    for (auto it1 = quorumLostFailoverUnits_.begin(); it1 != quorumLostFailoverUnits_.end(); it1++)
    {
        quorumLostWriter.Write("{0} ", *it1);
    }

    if(!quorumLostTextResult.empty())
    {
        TestSession::WriteInfo(TraceSource, "QuorumLost for FailoverUnits: {0}", quorumLostTextResult);
    }

    wstring rebuildLostTextResult;
    StringWriter rebuildLostWriter(rebuildLostTextResult);
    for (auto it1 = rebuildLostFailoverUnits_.begin(); it1 != rebuildLostFailoverUnits_.end(); it1++)
    {
        rebuildLostWriter.Write("{0} ", *it1);
    }

    if(!rebuildLostTextResult.empty())
    {
        TestSession::WriteInfo(TraceSource, "Rebuild lost for FailoverUnits: {0}", rebuildLostTextResult);
    }

    wstring toBeDeletedTextResult;
    StringWriter toBeDeletedWriter(toBeDeletedTextResult);
    for (auto it1 = toBeDeletedFailoverUnits_.begin(); it1 != toBeDeletedFailoverUnits_.end(); it1++)
    {
        toBeDeletedWriter.Write("{0} ", *it1);
    }

    if(!toBeDeletedTextResult.empty())
    {
        TestSession::WriteInfo(TraceSource, "ToBeDeleted FailoverUnits: {0}", toBeDeletedTextResult);
    }
}

bool FabricTestDispatcher::IsQuorumLostFU(Guid const& fuId)
{
    AcquireReadLock grab(quorumAndRebuildLostFailoverUnitsLock_);
    auto iterator = find(quorumLostFailoverUnits_.begin(), quorumLostFailoverUnits_.end(), fuId);
    return iterator != quorumLostFailoverUnits_.end();
}

bool FabricTestDispatcher::IsRebuildLostFU(Guid const& fuId)
{
    AcquireReadLock grab(quorumAndRebuildLostFailoverUnitsLock_);
    auto iterator = find(rebuildLostFailoverUnits_.begin(), rebuildLostFailoverUnits_.end(), fuId);
    return iterator != rebuildLostFailoverUnits_.end();
}

void FabricTestDispatcher::GetDownNodesForFU(Common::Guid const& fuId, vector<NodeId> & nodeIds)
{
    auto fm = GetFM();
    TestSession::FailTestIf(!fm, "FM not ready");

    auto fuPtr = fm->FindFTByPartitionId(fuId);
    if (fuPtr == nullptr)
    {
        return;
    }

    auto replicas = fuPtr->GetReplicas();
    for (auto replica = replicas.begin(); replica != replicas.end(); ++replica)
    {
        // Verify if this replica is down and might contribute to Failover-Unit quorum-lost state.
        // e.i. it's part of (replicaset -- not IsDropped) and it's not up/running right now.
        if (!replica->IsDropped && !replica->IsUp)
        {
            nodeIds.push_back(replica->FederationNodeId);
        }
    }
}

// This function will check if any FailoverUnit of the service "serviceName" is in quorumlost state.
// If yes then Ids of nodes which are down/inactive causing quorumlost state will be added to the set downNodeIds/deactivatedNodeIds.
void FabricTestDispatcher::RemoveFromQuorumLoss(std::wstring const& serviceName, set<NodeId> & downNodeIds, set<NodeId> & deactivatedNodeIds)
{
    UpdateQuorumOrRebuildLostFailoverUnits();

    vector<Common::Guid> fus;
    ServiceMap.GetAllFUsForService(serviceName, fus);
    for(auto iter = fus.begin(); iter != fus.end(); iter++)
    {
        TestSession::WriteNoise(
            TraceSource,
            "RemoveFromQuorumLoss: Checking quorum lost for FailoverUnit {0}:{1}",
            serviceName,
            *iter);
        if(IsQuorumLostFU(*iter))
        {
            vector<NodeId> nodeIds;
            GetDownNodesForFU(*iter, nodeIds);
            for(auto nodeId : nodeIds)
            {
                // Test if this node is Down or Deactivated.
                // Note: The trace messages will be printed for the same node for each FailoverUnits.
                if(!testFederation_.ContainsNode(nodeId))
                {
                    // This node is "Down" (Removed from the ring). In this case node needs to be brought back to the ring so that replica on this node can come up.
                    TestSession::WriteInfo(
                        TraceSource,
                        "RemoveFromQuorumLoss: Node {0} needs to be brought back to get service {1}:{2} out of Quorum loss",
                        nodeId,
                        serviceName,
                        *iter);

                    // Add in the list of down nodes.
                    downNodeIds.insert(nodeId);
                }
                else if ( !testFederation_.GetNode(nodeId)->IsActivated )
                {
                    // This node has been deactivated. In this case node needs to be activated so that replicas on this node can come up.
                    TestSession::WriteInfo(
                        TraceSource,
                        "RemoveFromQuorumLoss: Node {0} needs to be activated to get service {1}:{2} out of Quorum loss",
                        nodeId,
                        serviceName,
                        *iter);

                    // Add in the list of deactivated nodes.
                    deactivatedNodeIds.insert(nodeId);
                }
                else
                {
                    // This node is not up and is not deactivated. It is just coming "UP" from down state. In this case nothing to be done for this node as replica on this node will come up too.
                    TestSession::WriteInfo(
                        TraceSource,
                        "RemoveFromQuorumLoss: Node {0} is activated but not in the ring(testFederation) currently. This node is just joining the ring. Replicas on this node is expected to be up soon.",
                        nodeId);
                }
            }
        }
        else
        {
            TestSession::WriteNoise(
                TraceSource,
                "RemoveFromQuorumLoss: Quorum not lost for FailoverUnit {0}:{1}",
                serviceName,
                *iter);
        }
    }
}

bool FabricTestDispatcher::VerifyFMReplicaActivationStatus(ReplicaSnapshot & replica, FailoverUnitId const & fuId)
{
    if (replica.IsUp)
    {
        FabricTestNodeSPtr fabricNode = testFederation_.GetNode(replica.FederationNodeId);
        if (!fabricNode)
        {
            // Node has gone down - FM does not know yet
            return true;
        }

        if (!fabricNode->IsActivated)
        {
            TestSession::WriteInfo(TraceSource, "Replica {0} for {1} on {2} is Up when node is deactivated",
                replica.InstanceId,
                fuId,
                replica.FederationNodeId);
            return false;
        }
    }

    return true;
}

bool FabricTestDispatcher::VerifyGFUM(
    map<FailoverUnitId, FailoverUnitSnapshot> const & failoverUnits,
    vector<ServiceSnapshot> const & services)
{
    for (auto it = services.begin() ; it != services.end(); it++ )
    {
        if(ServiceMap.IsCreatedService(it->Name))
        {
            if(it->IsDeleted || it->IsToBeDeleted)
            {
                TestSession::WriteInfo(TraceSource, "Active Service {0} is marked IsDeleted = {1} and IsToBeDeleted {2}",
                    it->Name,
                    it->IsDeleted,
                    it->IsToBeDeleted);
                return false;
            }
        }
        else if (ServiceMap.IsDeletedService(it->Name))
        {
            if (it->IsToBeDeleted)
            {
                bool isQuorumLost = false;
                if(FabricTestSessionConfig::GetConfig().SkipDeleteVerifyForQuorumLostServices)
                {
                    vector<Common::Guid> fus;
                    ServiceMap.GetAllFUsForService(it->Name, fus);
                    for(auto iter = fus.begin(); iter != fus.end(); iter++)
                    {
                        if(IsQuorumLostFU(*iter))
                        {
                            isQuorumLost = true;
                            break;
                        }
                    }
                }

                if(isQuorumLost)
                {
                    TestSession::WriteInfo(TraceSource, "Service {0} is in QuorumLost and ToBeDeleted. Skipping verify for this service", it->Name);
                }
                else
                {
                    TestSession::WriteInfo(TraceSource, "Service {0} in ToBeDeleted state. Need to Wait for FM state machine to run", it->Name);
                    return false;
                }
            }

            TestSession::FailTestIfNot(it->IsDeleted || it->IsToBeDeleted,
                "Service {0} should be either marked as IsToBeDeleted or IsDeleted but is neither: {1}",
                it->Name, *it);
        }
        else if (it->Name != Reliability::Constants::TombstoneServiceName && !ServiceModel::SystemServiceApplicationNameHelper::IsNamespaceManagerName(it->Name))
        {
            TestSession::WriteInfo(TraceSource, "Service {0} should be created or deleted", it->Name);
            return false;
        }
    }

    vector<ServiceDescription> serviceDescriptions;
    ServiceMap.GetCreatedServicesDescription(serviceDescriptions);
    for (auto it = serviceDescriptions.begin() ; it != serviceDescriptions.end(); it++ )
    {
        ServiceDescription const& testServiceDescription = (*it);
        auto gfumService = std::find_if(services.begin(), services.end(), [&] (ServiceSnapshot const & inner)
        {
            return inner.Name == testServiceDescription.Name;
        });

        if(gfumService == services.end())
        {
            if (StringUtility::Compare(testServiceDescription.Name, SystemServiceApplicationNameHelper::InternalFMServiceName) == 0)
            {
                continue;
            }
            else if (FabricTestSessionConfig::GetConfig().AllowServiceAndFULossOnRebuild)
            {
                TestSession::WriteWarning(TraceSource, "{0} service not found in GFUM. Allowing due to rebuild", testServiceDescription.Name);
                continue;
            }
            else
            {
                wstring existingServiceNames;
                StringWriter writer(existingServiceNames);
                for (auto serviceIter = services.begin(); serviceIter != services.end(); ++serviceIter)
                {
                    writer.Write("{0} ", serviceIter->Name);
                }

                TestSession::WriteInfo(TraceSource, "{0} service not found in GFUM. Existing services are {1}", testServiceDescription.Name, existingServiceNames);
                return false;
            }
        }

        TestSession::FailTestIf(gfumService->IsDeleted || gfumService->IsToBeDeleted,
            "Service {0} should neither be marked IsToBeDeleted or IsDeleted",
            testServiceDescription.Name);
        vector<FailoverUnitSnapshot> serviceFailoverUnits;

        for (auto it1 = failoverUnits.begin(); it1 != failoverUnits.end(); it1++)
        {
            auto ftServiceInfo = it1->second.GetServiceInfo();
            if (ftServiceInfo.Name == testServiceDescription.Name)
            {
                serviceFailoverUnits.push_back(it1->second);
            }
        }

        // If auto scaling is enabled, then PLB will initiate repartitioning from inside SF and ServiceMap won't know about it.
        // In case that it is enabled, do not check if number of FTs will match
        if (testServiceDescription.ScalingPolicies.size() == 0 && static_cast<int>(serviceFailoverUnits.size()) != testServiceDescription.PartitionCount)
        {
            if(FabricTestSessionConfig::GetConfig().AllowServiceAndFULossOnRebuild)
            {
                TestSession::WriteWarning(TraceSource, "Failover units for Service {0} did not match. Allowing due to rebuild", testServiceDescription.Name);
                continue;
            }
            else
            {
                // This is a workarond for Bug#886396 - FailoverUnit for successfully deleted service coming back to life
                // Basically FM needs to handle this scenario where a FailoverUnit from a deleted service comes back because of rebuild
                // from a previously down replica. This is planed for RTM
                if(serviceFailoverUnits.size() > static_cast<size_t>(testServiceDescription.PartitionCount) && FabricTestSessionConfig::GetConfig().AllowUnexpectedFUs)
                {
                    TestSession::WriteInfo(TraceSource, "Extra Failover units for Service {0} detected. Allowing due to AllowUnexpectedFUs set", testServiceDescription.Name);
                }
                else
                {
                    TestSession::WriteInfo(TraceSource, "Failover units for Service {0} did not match. Need to Wait for FM state machine to run. Expected {1}, Actual {2}",
                        testServiceDescription.Name,
                        testServiceDescription.PartitionCount,
                        serviceFailoverUnits.size());
                    return false;
                }
            }
        }

        for (auto serviceFailoverUnitIter = serviceFailoverUnits.begin(); serviceFailoverUnitIter != serviceFailoverUnits.end(); serviceFailoverUnitIter++)
        {
            if (serviceFailoverUnitIter->IsCreatingPrimary)
            {
                TestSession::WriteInfo(TraceSource, "FM is creating primary {0}", *serviceFailoverUnitIter);
                return false;
            }

            if(!serviceFailoverUnitIter->IsQuorumLost())
            {
                TestSession::FailTestIf(
                    testServiceDescription.ScalingPolicies.size() == 0 && serviceFailoverUnitIter->IsToBeDeleted,
                    "FailoverUnit {0} should not be marked IsToBeDeleted",
                    serviceFailoverUnitIter->Id);

                if (serviceFailoverUnitIter->IsChangingConfiguration)
                {
                    bool isQuorumLost = IsQuorumLostForReconfigurationFU(*serviceFailoverUnitIter);

                    if(!isQuorumLost)
                    {
                        ReconfigurationAgentComponentTestApi::FailoverUnitSnapshotUPtr primaryRafu;
                        TryGetPrimaryRAFU(*serviceFailoverUnitIter, primaryRafu);
                        TestSession::WriteInfo(
                            TraceSource,
                            "{0} is changing configuration",
                            serviceFailoverUnitIter->Id);
                        if(primaryRafu)
                        {
                            TestSession::WriteInfo(
                                TraceSource,
                                "Current RAFU is {0}",
                                *primaryRafu);
                        }

                        return false;
                    }
                    else
                    {
                        // Quorum lost for this FailoverUnit so continue and skip validation
                        continue;
                    }
                }

                int replicaCount = 0;
                auto replicas = serviceFailoverUnitIter->GetReplicas();
                for (auto replica= replicas.begin(); replica != replicas.end(); ++replica)
                {
                    if (Reliability::LoadBalancingComponent::PLBConfig::GetConfig().ConstraintCheckEnabled)
                    {
                        TestSession::FailTestIf(
                            ServiceMap.IsBlockedNode(testServiceDescription.Name, replica->FederationNodeId),
                            "Node {0} is in block list for {1}",
                            testServiceDescription.Name,
                            replica->FederationNodeId);
                    }

                    if (!VerifyFMReplicaActivationStatus(*replica, serviceFailoverUnitIter->Id))
                    {
                        return false;
                    }

                    if (replica->IsUp && !replica->IsStandBy)
                    {
                        FabricTestNodeSPtr fabricNode = testFederation_.GetNode(replica->FederationNodeId);
                        if (!fabricNode || replica->IsInBuild)
                        {
                            TestSession::WriteInfo(TraceSource, "Replica {0} for ServiceName: {1}/FTId: {2} on {3} is down or in-build",
                                replica->InstanceId,
                                testServiceDescription.Name,
                                serviceFailoverUnitIter->Id,
                                replica->FederationNodeId);
                            return false;
                        }

                        TestServiceInfo testServiceInfo(
                            testServiceDescription.ApplicationId.ToString(),
                            testServiceDescription.Type.ToString(),
                            testServiceDescription.Name,
                            serviceFailoverUnitIter->Id.ToString(),
                            replica->ServiceLocation,
                            testServiceDescription.IsStateful,
                            Reliability::ReplicaRole::ConvertToPublicReplicaRole(replica->CurrentConfigurationRole));
                        fabricNode->RegisterServiceForVerification(testServiceInfo);

                        replicaCount++;
                    }
                }

                int blockListCount = static_cast<int>(ServiceMap.GetBlockListSize(testServiceDescription.Name));
                TestSession::FailTestIf(blockListCount > testFederation_.Count, "BlockList {0} cannot be greater than federation {1}", blockListCount, testFederation_.Count);

                if (testServiceDescription.TargetReplicaSetSize == -1 && replicaCount != (testFederation_.Count - blockListCount))
                {
                    TestSession::WriteInfo(TraceSource, "Number of replica does not match desired for Service {0} and FUID {1} with -1 instance count. Expected {2}, Actual {3}, Node Count {4}, Block List Count {5}",
                        testServiceDescription.Name,
                        serviceFailoverUnitIter->Id,
                        testFederation_.Count - blockListCount,
                        replicaCount,
                        testFederation_.Count,
                        blockListCount);
                    return false;
                }
                else if (replicaCount != testServiceDescription.TargetReplicaSetSize &&
                    replicaCount < testFederation_.Count &&
                    replicaCount < (testFederation_.Count - blockListCount))
                {
                    TestSession::WriteInfo(TraceSource, "Number of replica does not match desired for Service {0} and FUID {1}. Expected {2}, Actual {3}, Node Count {4}",
                        testServiceDescription.Name,
                        serviceFailoverUnitIter->Id,
                        testServiceDescription.TargetReplicaSetSize,
                        replicaCount,
                        testFederation_.Count);

                    return false;
                }
            }
        }
    }

    return true;
}

FailoverManagerTestHelperUPtr FabricTestDispatcher::VerifyFM()
{
    FabricTestNodeSPtr const & fmNode = testFederation_.GetNodeClosestToZero();
    if (!fmNode)
    {
        TestSession::WriteInfo(TraceSource, "No node in the federation");
        return nullptr;
    }

    auto fm = GetFM();
    if (!fm)
    {
        TestSession::WriteInfo(TraceSource, "FM is not ready");
        return nullptr;
    }

    NodeId fmId;
    fmId = fm->FederationNodeId;

    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        bool fmNotNull = node->Node->GetFailoverManager() != nullptr;

        if (node->Id != fmId && fmNotNull)
        {
            TestSession::WriteInfo(TraceSource, "{0} is not deactivated from FM role", node->Id);
            return nullptr;
        }
    }

    return fm;
}

bool FabricTestDispatcher::VerifyFMM()
{
    return VerifyFMM(-1);
}

bool FabricTestDispatcher::VerifyFMM(int expectedReplicaCount)
{
    auto fm = GetFMM();
    if(!fm)
    {
        TestSession::WriteInfo(TraceSource, "FMM not ready");
        return false;
    }

    auto services = fm->SnapshotServiceCache();

    if (services.size() == 0)
    {
        TestSession::WriteInfo(TraceSource, "FM replica set not recovered");
        return false;
    }

    auto service = services.data();

    auto failoverUnits = fm->SnapshotGfum();

    if (failoverUnits.size() == 0)
    {
        TestSession::WriteInfo(TraceSource, "FM replica set not recovered");
        return false;
    }

    TestSession::FailTestIfNot(failoverUnits.size() == 1, "FMM contains {0} FailoverUnit", failoverUnits.size());
    TestSession::FailTestIfNot(service->ServiceDescription.PartitionCount == 1, "FM service PartitionCount does not match: {0}", *service);
    TestSession::FailTestIfNot(service->Name == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName, "FM service name does not match: {0}", *service);
    TestSession::FailTestIf(service->IsDeleted || service->IsToBeDeleted, "FM service is marked for deletion: {0}", *service);

    auto failoverUnit = failoverUnits.data();
    if(!failoverUnit->IsQuorumLost())
    {
        TestSession::FailTestIf(failoverUnit->IsToBeDeleted,
            "FM FailoverUnit {0} should not be marked IsToBeDeleted",
            failoverUnit->Id);

        if (failoverUnit->IsChangingConfiguration)
        {
            TestSession::WriteInfo(TraceSource, "FMM: {0} is changing configuration",
                failoverUnit->Id);
            return false;
        }

        int replicaCount = 0;
        auto ftReplicas = failoverUnit->GetReplicas();
        for (auto it2 = ftReplicas.begin(); it2 != ftReplicas.end(); it2++)
        {
            ReplicaSnapshot & replica = *it2;
            if (replica.IsUp && !replica.IsStandBy)
            {
                FabricTestNodeSPtr fabricNode = testFederation_.GetNode(replica.FederationNodeId);
                if (!fabricNode || replica.IsInBuild)
                {
                    TestSession::WriteInfo(TraceSource, "FMM: Replica {0} for ServiceName: {1}/FTId: {2} on {3} is down or in-build",
                        replica.InstanceId,
                        service->Name,
                        failoverUnit->Id,
                        replica.FederationNodeId);
                    return false;
                }

                replicaCount++;
            }
        }

        if (expectedReplicaCount < 0)
        {
            expectedReplicaCount = service->ServiceDescription.TargetReplicaSetSize;
        }

        if (replicaCount != expectedReplicaCount &&
            replicaCount < testFederation_.Count)
        {
            TestSession::WriteInfo(TraceSource, "FMM: Number of replicas does not match desired for Service {0} and FUID {1}. Expected {2}, Actual {3}, Node Count {4}",
                service->Name,
                failoverUnit->Id,
                expectedReplicaCount,
                replicaCount,
                testFederation_.Count);
            return false;
        }
    }

    return true;
}

bool FabricTestDispatcher::VerifyNodes(vector<NodeSnapshot> const & nodes, size_t upCount, wstring const& fmType)
{
    set<NodeId> upNodes;
    wstring result;
    StringWriter writer(result);

    uint64 expectedUpCount = 0;

    for(auto node = nodes.begin(); node != nodes.end(); ++node)
    {
        if (node->IsUp)
        {
            expectedUpCount++;

            upNodes.insert(node->NodeInstance.Id);

            FabricTestNodeSPtr const & testNode = testFederation_.GetNode(node->NodeInstance.Id);
            if (!testNode)
            {
                writer.Write("(Node {0} expected to be down at {1})", node->NodeInstance, fmType);
            }
            else
            {
                testNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
                {
                    auto reliabilityNodeInstance = reliabilityTestHelper.Reliability.FederationWrapper.Instance;
                    if (reliabilityNodeInstance != node->NodeInstance)
                    {
                        writer.Write("(Stale node instance {0}/{1} at {2})", reliabilityNodeInstance, node->NodeInstance, fmType);
                    }
                });

                bool isDeactivated = node->DeactivationInfo.IsRestart;
                if (isDeactivated && testNode->IsActivated)
                {
                    TestSession::WriteInfo(TraceSource, "Node {0} Activation status mismatch. {1} State {2}. FabricTest: {3}", node->Id, fmType, node->DeactivationInfo.Intent, testNode->IsActivated);
                    return false;
                }
            }
        }
    }

    if (upCount != expectedUpCount)
    {
        writer.Write("Node UpCount does not match at {0} the actual count: Expected: {1}, Actual: {2}", fmType, expectedUpCount, upCount);
        return false;
    }

    for (FabricTestNodeSPtr const & testNode : testFederation_.GetNodes())
    {
        if (upNodes.find(testNode->Id) == upNodes.end())
        {
            writer.Write("(Node {0} expected to be up at {1})", testNode->Id, fmType);
        }
    }

    if (result.size() > 0)
    {
        TestSession::WriteInfo(TraceSource, "{0}", result);
        return false;
    }

    return true;
}

wstring FabricTestDispatcher::GetWorkingDirectory(wstring const & id)
{
    wstring nodesDir = Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestNodesDirectory);
    return Path::Combine(nodesDir, id);
}

void FabricTestDispatcher::RemoveTicketFile(wstring const & id)
{
    wstring path = Path::Combine(GetWorkingDirectory(id), id + L".tkt");
    File::Delete(path, false);
}

bool FabricTestDispatcher::ResetStore(StringCollection const & params)
{
    if (params.size() == 0)
    {
        ErrorCode error = ReliabilityTestHelper::DeleteAllFMStoreData();
        TestSession::FailTestIfNot(error.IsSuccess(), "Delete of FMStore data failed with {0}", error);

        DeleteDirectoryWithRetry(FabricTestDispatcher::TestDataDirectory);

        CreateDirectoryWithRetry(FabricTestDispatcher::TestDataDirectory);

        if (FabricTestCommonConfig::GetConfig().IsBackupTest)
        {
            //Copy the source backup files over to the test directory
            if (!Directory::Exists(FabricTestConstants::TestBackupDirectory))
            {
                CreateDirectoryWithRetry(FabricTestConstants::TestBackupDirectory);

                wstring pathToSrcBackup = Path::Combine(Environment::GetExecutablePath(), FabricTestConstants::TestBackupDirectory);
                bool overWrite = true;

                if (Directory::Exists(pathToSrcBackup))
                {
                    Directory::SafeCopy(pathToSrcBackup,FabricTestConstants::TestBackupDirectory,overWrite);
                }
                else
                {
#if !defined(PLATFORM_UNIX)
                     TestSession::FailTest("Source backup files do not exist! {0}", pathToSrcBackup);
#endif
                }
            }
        }

        Directory::Create(Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestNodesDirectory));
        Directory::Create(Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestImageStoreDirectory));
        Directory::Create(Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestTracesDirectory));
        std::wstring testHostTraceDirectory = Path::Combine(FabricTestDispatcher::TestDataDirectory, Path::Combine(FabricTestConstants::TestTracesDirectory, FabricTestConstants::TestHostTracesDirectory));
        Directory::Create(testHostTraceDirectory);
#if defined (PLATFORM_UNIX)
        //so that tests run with sudo can access this directory
        string testHostTraceDirectoryUtf8;
        StringUtility::Utf16ToUtf8(testHostTraceDirectory, testHostTraceDirectoryUtf8);
        string cmd = "chmod -R 777 " + testHostTraceDirectoryUtf8;
        system(cmd.c_str());
#endif
        Directory::Create(Path::Combine(FabricTestDispatcher::TestDataDirectory, Path::Combine(FabricTestConstants::TestTracesDirectory, FabricTestConstants::TestNodesDirectory)));

        auto testDataRoot = Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestDataRootDirectory);
        Directory::Create(testDataRoot);
        Environment::SetEnvironmentVariableW(Common::FabricEnvironment::FabricDataRootEnvironmentVariable, testDataRoot);

        if (this->IsNativeImageStoreEnabled)
        {
            Management::ManagementConfig::GetConfig().ImageStoreConnectionString = SecureString(*Management::ImageStore::Constants::NativeImageStoreSchemaTag);
        }
        else
        {
            wstring imageStoreConnectionString = Path::Combine(L"file:.", Path::Combine(Path::GetFileName(FabricTestDispatcher::TestDataDirectory), FabricTestConstants::TestImageStoreDirectory));
            Management::ManagementConfig::GetConfig().ImageStoreConnectionString = SecureString(imageStoreConnectionString);
        }

        Hosting2::HostingConfig::GetConfig().DllHostExePath = wformatString(
            "FabricTestHost.exe testdir={0} serverport={1} useetw={2} security=None",
            FabricTestDispatcher::TestDataDirectory,
            FederationTestCommon::AddressHelper::ServerListenPort,
            FabricTestSessionConfig::GetConfig().UseEtw);
    }
    else
    {
        CommandLineParser parser(params);
        StringCollection nodes;
        parser.GetVector(L"nodes", nodes);

        wstring dirType;
        parser.TryGetString(L"type", dirType);

        for (wstring const & id : nodes)
        {
            wstring path = Path::Combine(GetWorkingDirectory(id), dirType);
            DeleteDirectoryWithRetry(path);
        }
    }

    return true;
}

bool FabricTestDispatcher::CleanTest()
{
    ClearTicket(StringCollection());
    ResetStore(StringCollection());
    return true;
}

bool FabricTestDispatcher::VerifyImageStore(StringCollection const & params)
{
    UNREFERENCED_PARAMETER(params);

    if(this->IsNativeImageStoreEnabled)
    {
        // If native ImageStore is enabled, then return
        return true;
    }

    wstring imageStoreRoot = Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestImageStoreDirectory);

    Management::ImageModel::StoreLayoutSpecification layout(imageStoreRoot);

    vector<wstring> files = Directory::GetFiles(layout.Root);
    if (files.size() > 0)
    {
        TestSession::FailTest("File {0} still exists under {1}", files, imageStoreRoot);
    }

    return true;
}

bool FabricTestDispatcher::VerifyNodeFiles(StringCollection const & params)
{
    UNREFERENCED_PARAMETER(params);

    if (this->IsNativeImageStoreEnabled)
    {
        // If native ImageStore is enabled, then return
        return true;
    }

    wstring imageStoreRoot = Path::Combine(FabricTestDispatcher::TestDataDirectory, FabricTestConstants::TestImageStoreDirectory);

    set<wstring> imageStoreFiles;
    vector<wstring> files = Directory::GetFiles(Path::Combine(imageStoreRoot, L"Store"), L"*.*", true, false);
    for (auto iter = files.begin(); iter != files.end(); ++iter)
    {
        StringUtility::Replace<wstring>(*iter, imageStoreRoot, L"");
#if !defined(PLATFORM_UNIX)
        StringUtility::TrimLeading<wstring>(*iter, L"\\");
#else
        StringUtility::TrimLeading<wstring>(*iter, L"/");
#endif
        imageStoreFiles.insert(*iter);
    }

    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        node->VerifyImageCacheFiles(imageStoreFiles);
        node->VerifyApplicationInstanceAndSharedPackageFiles(imageStoreFiles);
    }

    return true;
}

static Global<InstallTestCertInScope> defaultClusterCert;
static Global<InstallTestCertInScope> defaultClientCert;
static Global<InstallTestCertInScope> adminClientCert;

static void CertCleanupCallback()
{
    FABRICSESSION.FabricDispatcher.CleanupTestCerts();
}

bool FabricTestDispatcher::NewTestCerts()
{
    defaultClusterCert = make_global<InstallTestCertInScope>(true, L"CN=cluster");
    defaultClientCert = make_global<InstallTestCertInScope>(true, L"CN=DefaultClient");
    adminClientCert = make_global<InstallTestCertInScope>(true, L"CN=AdminClient");
    ZeroRetValAssert(atexit(CertCleanupCallback));

    auto clusterCertThumbprint = defaultClusterCert->Thumbprint()->PrimaryToString();
    auto clientCertThumbprint = defaultClientCert->Thumbprint()->PrimaryToString();
    auto adminClientCertThumbprint = adminClientCert->Thumbprint()->PrimaryToString();

    auto clusterCredentials = wformatString(
        "X509:EncryptAndSign:Empty:Empty:Empty:{0}:{1}:{0}:{2}",
        clusterCertThumbprint,
        adminClientCertThumbprint,
        clientCertThumbprint);

    auto defaultNodeCredentials = wformatString(
        "FindByThumbprint,{0},{1},{2}",
        clusterCertThumbprint, X509Default::StoreName(), X509Default::StoreLocation());

    *ClientSecurityHelper::DefaultSslClientCredentials = wformatString(
        "X509:{0}:{1}:FindByThumbprint:{2}:EncryptAndSign:::{3}",
        X509Default::StoreLocation(),
        X509Default::StoreName(),
        adminClientCertThumbprint,
        clusterCertThumbprint);

    wstring clientCredentials = ClientSecurityHelper::DefaultSslClientCredentials;

    if (!FABRICSESSION.FabricDispatcher.SetClusterWideCredentials(clusterCredentials)) return false;
    if (!FABRICSESSION.FabricDispatcher.SetDefaultNodeCredentials(defaultNodeCredentials)) return false;
    if (!FABRICSESSION.FabricDispatcher.SetClientCredentials(clientCredentials)) return false;

    return true;
}

void FabricTestDispatcher::CleanupTestCerts()
{
    if (defaultClusterCert) defaultClusterCert->Uninstall();
    if (defaultClientCert) defaultClientCert->Uninstall();
    if (adminClientCert) adminClientCert->Uninstall();
}

bool FabricTestDispatcher::UseAdminClientCredential()
{
    auto clientCredentials = wformatString(
        "X509:{0}:{1}:FindByThumbprint:{2}:EncryptAndSign:::{3}",
        X509Default::StoreLocation(),
        X509Default::StoreName(),
        adminClientCert->Thumbprint()->PrimaryToString(),
        defaultClusterCert->Thumbprint()->PrimaryToString());

    return FABRICSESSION.FabricDispatcher.SetClientCredentials(clientCredentials);
}

bool FabricTestDispatcher::UseReadonlyClientCredential()
{
    auto clientCredentials = wformatString(
        "X509:{0}:{1}:FindByThumbprint:{2}:EncryptAndSign:::{3}",
        X509Default::StoreLocation(),
        X509Default::StoreName(),
        defaultClientCert->Thumbprint()->PrimaryToString(),
        defaultClusterCert->Thumbprint()->PrimaryToString());

    return FABRICSESSION.FabricDispatcher.SetClientCredentials(clientCredentials);
}

bool FabricTestDispatcher::ClearTicket(StringCollection const & params)
{
    if (params.size() == 0)
    {
        for (auto seed : FederationConfig::GetConfig().Votes)
        {
            if (seed.Type == Federation::Constants::SeedNodeVoteType)
            {
                RemoveTicketFile(seed.Id.ToString());
            }
            else
            {
                Assert::CodingError("Sql Voters not supported");
/*
                auto store = SqlStore::Create(seed.ConnectionString);
                ErrorCode err = store->Open();
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                Store::ILocalStore::TransactionSPtr trans;
                err = store->CreateTransaction(trans);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Delete(trans, L"VoteOwnerData", L"", 0);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Delete(trans, L"SuperTicket", L"", 0);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Delete(trans, L"GlobalTicket", L"", 0);
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = trans->Commit();
                if (!err.IsSuccess())
                {
                    store->Abort();
                    return false;
                }

                err = store->Close();
                if (!err.IsSuccess())
                {
                    return false;
                }
*/
            }
        }
    }
    else
    {
        for (wstring const & id : params)
        {
            RemoveTicketFile(id);
        }
    }

    return true;
}

std::wstring FabricTestDispatcher::GetState(std::wstring const & param)
{
    vector<wstring> params;
    StringUtility::Split<wstring>(param, params, StatePropertyDelimiter);

    // FM
    if (params[0] == L"Primary" || StringUtility::StartsWith<wstring>(params[0], L"Secondary"))
    {
        return GetFailoverUnitLocationState(params);
    }
    else if (params[0] == L"FM")
    {
        return GetFMState(params);
    }
    else if (params[0] == L"FMM")
    {
        return GetFMMState(params);
    }
    else if (params[0] == L"RA")
    {
        return GetRAState(params);
    }
    else if (params[0] == L"CodePackId")
    {
        return GetCodePackageId(params);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetReplicaRoleString(ReplicaRole::Enum replicaRole)
{
    switch (replicaRole)
    {
    case ReplicaRole::Primary:
        return FabricTestDispatcher::ReplicaRolePrimary;
    case ReplicaRole::Secondary:
        return FabricTestDispatcher::ReplicaRoleSecondary;
    case ReplicaRole::Idle:
        return FabricTestDispatcher::ReplicaRoleIdle;
    default:
        return FabricTestDispatcher::ReplicaRoleNone;
    }
}

wstring FabricTestDispatcher::GetReplicaStateString(ReplicaStates::Enum replicaState)
{
    switch (replicaState)
    {
    case ReplicaStates::StandBy:
        return FabricTestDispatcher::ReplicaStateStandBy;
    case ReplicaStates::InBuild:
        return FabricTestDispatcher::ReplicaStateInBuild;
    case ReplicaStates::Ready:
        return FabricTestDispatcher::ReplicaStateReady;
    default:
        return FabricTestDispatcher::ReplicaStateDropped;
    }
}

wstring FabricTestDispatcher::GetReplicaStatusString(ReplicaStatus::Enum replicaStatus)
{
    switch (replicaStatus)
    {
    case ReplicaStatus::Down:
        return FabricTestDispatcher::ReplicaStatusDown;
    case ReplicaStatus::Up:
        return FabricTestDispatcher::ReplicaStatusUp;
    default:
        return FabricTestDispatcher::ReplicaStatusInvalid;
    }
}

wstring FabricTestDispatcher::GetServiceReplicaStatusString(FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus)
{
    switch (replicaStatus)
    {
    case FABRIC_QUERY_SERVICE_REPLICA_STATUS_INBUILD:
        return FabricTestDispatcher::ServiceReplicaStatusInBuild;
    case FABRIC_QUERY_SERVICE_REPLICA_STATUS_STANDBY:
        return FabricTestDispatcher::ServiceReplicaStatusStandBy;
    case FABRIC_QUERY_SERVICE_REPLICA_STATUS_READY:
        return FabricTestDispatcher::ServiceReplicaStatusReady;
    case FABRIC_QUERY_SERVICE_REPLICA_STATUS_DOWN:
        return FabricTestDispatcher::ServiceReplicaStatusDown;
    case FABRIC_QUERY_SERVICE_REPLICA_STATUS_DROPPED:
        return FabricTestDispatcher::ServiceReplicaStatusDropped;
    default:
        return FabricTestDispatcher::ServiceReplicaStatusInvalid;
    }
}

wstring FabricTestDispatcher::GetFailoverUnitLocationState(StringCollection const & params)
{
    if (params.size() == 2)
    {
        wstring const & serviceName = params[1];

        auto failoverUnit = GetFMFailoverUnitFromParam(serviceName);
        if (failoverUnit)
        {
            auto replicas = failoverUnit->GetCurrentConfigurationReplicas();

            if (params[0] == L"Primary")
            {
                auto primary = find_if(replicas.begin(), replicas.end(), [] (ReplicaSnapshot const & it)
                {
                    return it.CurrentConfigurationRole == ReplicaRole::Primary;
                });

                if (primary == replicas.end())
                {
                    return wstring();
                }

                return wformatString(primary->FederationNodeId);
            }

            size_t replicaIndex = 0;
            if (params[0].size() > 9)
            {
                replicaIndex = static_cast<size_t>(Common::Int32_Parse(params[0].substr(9)));
            }

            vector<NodeId> outputReplicas;
            for(auto replica = replicas.begin(); replica != replicas.end(); ++replica)
            {
                if (replica->CurrentConfigurationRole == ReplicaRole::Secondary)
                {
                    outputReplicas.push_back(replica->FederationNodeId);
                }
            }

            if (replicaIndex < outputReplicas.size())
            {
                sort(outputReplicas.begin(), outputReplicas.end());
                return wformatString(outputReplicas[replicaIndex]);
            }
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMState(StringCollection const & params)
{
    if (params.size() == 2)
    {
        auto fm = GetFM();
        if (fm)
        {
            if (params[1] == L"Location")
            {
                return wformatString(fm->FederationNodeId);
            }
            else if (params[1] == L"IsReady")
            {
                return wformatString(fm->IsReady);
            }
            else if (params[1] == L"HighestLookupVersion")
            {
                return wformatString(fm->HighestLookupVersion);
            }
            else if (params[1] == L"SavedLookupVersion")
            {
                return wformatString(fm->SavedLookupVersion);
            }
            else if (params[1] == L"FabricUpgradeTargetVersion")
            {
                return wformatString(fm->FabricUpgradeTargetVersion);
            }
        }
    }
    else if (params.size() > 2)
    {
        if (params[1] == L"Node")
        {
            return GetFMNodeState(params);
        }
        else if (params[1] == L"Replica")
        {
            return GetFMReplicaState(params);
        }
        else if (params[1] == L"UpReplicaCount")
        {
            return GetFMUpReplicaCount(params);
        }
        else if (params[1] == L"OfflineReplicaCount")
        {
            return GetFMOfflineReplicaCount(params);
        }
        else if (params[1] == L"Service")
        {
            return GetFMServiceState(params);
        }
        else if (params[1] == L"FT")
        {
            return GetFMFailoverUnitState(params);
        }
        else if (params[1] == L"App")
        {
            return GetFMApplicationState(params);
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMMState(StringCollection const & params)
{
    auto fmm = GetFMM();
    if(params.size() >= 2 && fmm)
    {
        if (params[1] == L"Location")
        {
            return wformatString(fmm->FederationNodeId);
        }
        else if (params[1] == L"Node")
        {
            NodeId nodeId = ParseNodeId(params[2]);
            auto nodeInfo = fmm->FindNodeByNodeId(nodeId);
            if (nodeInfo)
            {
                return wformatString(nodeInfo->IsUp);
            }
        }
        else if (params[1] == L"Service")
        {
            return GetFMMServiceState(params);
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetRAState(StringCollection const & params)
{
    if(params.size() >= 2)
    {
        if (params[1] == L"FT")
        {
            return GetRAFUState(params);
        }
        else if (params[1] == L"Replica")
        {
            return GetRAReplicaState(params);
        }
        else if (params[1] == L"Upgrade")
        {
            return GetRAUpgradeState(params);
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMFailoverUnitState(StringCollection const & params)
{
    if (params.size() == 4)
    {
        if (params[2] == L"Reconfig")
        {
            return GetReconfig(params);
        }
        else if (params[2] == L"QuorumLost")
        {
            return GetQuorumLost(params);
        }
        else if (params[2] == L"IsInRebuild")
        {
            return GetIsInRebuild(params);
        }
        else if (params[2] == L"PartitionId")
        {
            return GetPartitionId(params);
        }
        else if (params[2] == L"PartitionKind")
        {
            return GetPartitionKind(params);
        }
        else if (params[2] == L"PartitionLowKey")
        {
            return GetPartitionLowKey(params);
        }
        else if (params[2] == L"PartitionHighKey")
        {
            return GetPartitionHighKey(params);
        }
        else if (params[2] == L"PartitionName")
        {
            return GetPartitionHighKey(params);
        }
        else if (params[2] == L"TargetReplicaSetSize")
        {
            return GetTargetReplicaSetSize(params);
        }
        else if (params[2] == L"MinReplicaSetSize")
        {
            return GetMinReplicaSetSize(params);
        }
        else if (params[2] == L"Status")
        {
            return GetPartitionStatus(params);
        }
        else if (params[2] == L"DataLossVersion")
        {
            return GetDataLossVersion(params);
        }
        else if (params[2] == L"IsOrphaned")
        {
            return GetIsOrphaned(params);
        }
        else if (params[2] == L"DroppedReplicaCount")
        {
            return GetDroppedReplicaCount(params);
        }
        else if (params[2] == L"ReplicaCount")
        {
            return GetReplicaCount(params);
        }
        else if (params[2] == L"Epoch")
        {
            return GetEpoch(params);
        }
        else if (params[2] == L"Exists")
        {
            return GetFMFailoverUnitFromParam(params[3]) ? L"true" : L"false";
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMNodeState(StringCollection const & params)
{
    auto fm = GetFM();
    if (params.size() == 4 && fm)
    {
        NodeId nodeId = ParseNodeId(params[3]);
        auto nodeInfo = fm->FindNodeByNodeId(nodeId);

        if (params[2] == L"Exists")
        {
            return wformatString(nodeInfo != nullptr);
        }
        else if (nodeInfo)
        {
            if (params[2] == L"IsUp")
            {
                return wformatString(nodeInfo->IsUp);
            }
            else if (params[2] == L"Status")
            {
                return wformatString(nodeInfo->NodeStatus);
            }
            else if (params[2] == L"DeactivationIntent")
            {
                return wformatString(nodeInfo->DeactivationInfo.Intent);
            }
            else if (params[2] == L"DeactivationStatus")
            {
                return wformatString(nodeInfo->DeactivationInfo.Status);
            }
            else if (params[2] == L"InstanceId")
            {
                return wformatString(nodeInfo->NodeInstance.InstanceId);
            }
            else if (params[2] == L"Version")
            {
                return wformatString(nodeInfo->VersionInstance);
            }
            else if (params[2] == L"CodeVersion")
            {
                return wformatString(nodeInfo->VersionInstance.Version.CodeVersion);
            }
            else if (params[2] == L"ConfigVersion")
            {
                return wformatString(nodeInfo->VersionInstance.Version.ConfigVersion);
            }
            else if (params[2] == L"UpgradeDomain")
            {
                return wformatString(nodeInfo->ActualUpgradeDomainId);
            }
            else if (params[2] == L"IsReplicaUploaded")
            {
                return wformatString(nodeInfo->IsReplicaUploaded);
            }
            else if (params[2] == L"NodeName")
            {
                return wformatString(nodeInfo->NodeName);
            }
            else if (params[2] == L"NodeType")
            {
                return wformatString(nodeInfo->NodeType);
            }
            else if (params[2] == L"IpAddressOrFQDN")
            {
                return wformatString(nodeInfo->IpAddressOrFQDN);
            }
            else if (params[2] == L"IsSeedNode")
            {
                return wformatString(IsSeedNode(nodeId));
            }
            else if (params[2] == L"IsNodeStateRemoved")
            {
                return wformatString(nodeInfo->IsNodeStateRemoved);
            }
            else if (params[2] == L"IsUnknown")
            {
                return wformatString(nodeInfo->IsUnknown);
            }
            else if (params[2] == L"IsPendingFabricUpgrade")
            {
                return wformatString(nodeInfo->IsPendingFabricUpgrade);
            }
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMReplicaState(StringCollection const & params)
{
    if (params.size() == 5)
    {
        wstring const & serviceName = params[3];
        auto failoverUnit = GetFMFailoverUnitFromParam(serviceName);
        NodeId nodeId = ParseNodeId(params[4]);
        if(failoverUnit)
        {
            auto replica = failoverUnit->GetReplica(nodeId);
            if(replica)
            {
                if (params[2] == L"IsUp")
                {
                    return wformatString(replica->IsUp);
                }
                else if (params[2] == L"State")
                {
                    return GetReplicaStateString(replica->ReplicaState);
                }
                else if (params[2] == L"Version")
                {
                    return replica->Version.Version.ToString();
                }
                else if (params[2] == L"Role")
                {
                    return GetReplicaRoleString(replica->CurrentConfigurationRole);
                }
                else if (params[2] == L"Info")
                {
                    return wformatString("{0}#{1}", failoverUnit->Id, replica->ReplicaId);
                }
                else if (params[2] == L"InstanceId")
                {
                    return wformatString(replica->InstanceId);
                }
                else if (params[2] == L"IsToBeDropped")
                {
                    return wformatString(replica->IsToBeDropped);
                }
            }
            else
            {
                if (params[2] == L"State")
                {
                    return FabricTestDispatcher::ReplicaStateDropped;
                }
                else
                {
                    return wstring(L"NullReplica");
                }
            }
        }
        else
        {
            return wstring(L"NullFT");
        }
    }
    else if (params.size() == 4)
    {
        wstring const & serviceName = params[3];
        FailoverUnitSnapshotUPtr failoverUnit;
        auto replicaPtr = GetReplicaFromParam(serviceName, failoverUnit);
        if (replicaPtr != nullptr)
        {
            auto const & replica = *replicaPtr;

            if (params[2] == L"Id")
            {
                return wformatString(replica.ReplicaId);
            }
            else if (params[2] == L"Role")
            {
                return GetReplicaRoleString(replica.CurrentConfigurationRole);
            }
            else if (params[2] == L"Status")
            {
                return GetReplicaStatusString(replica.ReplicaStatus);
            }
            else if (params[2] == L"Address")
            {
                return wformatString(replica.ServiceLocation);
            }
            else if (params[2] == L"InstanceId")
            {
                return wformatString(replica.InstanceId);
            }
            else if (params[2] == L"NodeId")
            {
                return replica.FederationNodeId.IdValue.ToString();
            }
            else if (params[2] == L"NodeName")
            {
                return replica.GetNode().NodeName;
            }
            else if (params[2] == L"State")
            {
                return GetReplicaStateString(replica.ReplicaState);
            }
            else if (params[2] == L"NodeInstanceId")
            {
                return wformatString(replica.GetNode().NodeInstance.InstanceId);
            }
        }
        else
        {
            return wstring(L"InvalidReplica");
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMUpReplicaCount(StringCollection const& params)
{
    if (params.size() == 3)
    {
        NodeId nodeId = ParseNodeId(params[2]);

        auto fm = GetFM();
        if (!fm)
        {
            return wstring();
        }

        auto gfum = fm->SnapshotGfum();
        auto upReplicaCount = count_if(gfum.begin(), gfum.end(), [&] (FailoverUnitSnapshot const & ft)
        {
            auto replica = ft.GetReplica(nodeId);
            return replica != nullptr && replica->IsUp;
        });

        return wformatString(upReplicaCount);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMOfflineReplicaCount(StringCollection const& params)
{
    if (params.size() == 3)
    {
        NodeId nodeId = ParseNodeId(params[2]);

        auto fm = GetFM();
        if (!fm)
        {
            return wstring();
        }

        auto gfum = fm->SnapshotGfum();
        auto offlineReplicaCount = count_if(gfum.begin(), gfum.end(), [&] (FailoverUnitSnapshot const & ft)
        {
            auto replica = ft.GetReplica(nodeId);
            return replica != nullptr && replica->IsOffline;
        });

        return wformatString(offlineReplicaCount);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMApplicationState(StringCollection const& params)
{
    if (params.size() == 4)
    {
        auto fm = GetFM();
        if (!fm)
        {
            return wstring();
        }

        StringCollection tokens;
        StringUtility::Split<wstring>(params[3], tokens, L"_");
        StringUtility::TrimLeading<wstring>(tokens[1], L"App");

        ApplicationIdentifier appId(tokens[0], static_cast<ULONG>(Int64_Parse(tokens[1])));

        auto appInfo = fm->FindApplicationById(appId);
        if (!appInfo)
        {
            return L"NULL";
        }
        
        if (params[2] == L"IsDeleted")
        {
            return wformatString(appInfo->IsDeleted);
        }

        if (params[2] == L"IsUpgradeCompleted")
        {
            return wformatString(appInfo->IsUpgradeCompleted);
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMServiceState(StringCollection const & params)
{
    if (params.size() == 4)
    {
        auto fm = GetFM();
        if(!fm)
        {
            return wstring();
        }

        wstring const & serviceName = params[3];

        auto service = fm->FindServiceByServiceName(serviceName);
        if (service == nullptr)
        {
            return L"NULL";
        }

        if (params[2] == L"Version")
        {
            return service->ServiceDescription.PackageVersion.ToString();
        }
        else if (params[2] == L"UpgradeVersion")
        {
            ServiceModel::ServicePackageVersionInstance version = service->GetTargetVersion(wstring());
            return version.Version.ToString();
        }
        else if (params[2] == L"TargetReplicaSetSize")
        {
            return wformatString(service->ServiceDescription.TargetReplicaSetSize);
        }
        else if (params[2] == L"MinReplicaSetSize")
        {
            return wformatString(service->ServiceDescription.MinReplicaSetSize);
        }
        else if (params[2] == L"ReplicaRestartWaitDuration")
        {
            return wformatString(service->ServiceDescription.ReplicaRestartWaitDuration.TotalSeconds());
        }
        else if (params[2] == L"QuorumLossWaitDuration")
        {
            return wformatString(service->ServiceDescription.QuorumLossWaitDuration.TotalSeconds());
        }
        else if (params[2] == L"StandByReplicaKeepDuration")
        {
            return wformatString(service->ServiceDescription.StandByReplicaKeepDuration.TotalSeconds());
        }
        else if (params[2] == L"UpdateVersion")
        {
            return wformatString(service->ServiceDescription.UpdateVersion);
        }
        else if (params[2] == L"IsDeleted")
        {
            return wformatString(service->IsDeleted);
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetFMMServiceState(StringCollection const & params)
{
    if (params.size() == 4)
    {
        auto fm = GetFMM();
        if (!fm)
        {
            return wstring();
        }

        wstring const & serviceName = params[3];

        auto service = fm->FindServiceByServiceName(serviceName);
        if (service == nullptr)
        {
            return wstring();
        }

        if (params[2] == L"TargetReplicaSetSize")
        {
            return wformatString(service->ServiceDescription.TargetReplicaSetSize);
        }
        else if (params[2] == L"MinReplicaSetSize")
        {
            return wformatString(service->ServiceDescription.MinReplicaSetSize);
        }
        else if (params[2] == L"ReplicaRestartWaitDuration")
        {
            return wformatString(service->ServiceDescription.ReplicaRestartWaitDuration.TotalSeconds());
        }
        else if (params[2] == L"QuorumLossWaitDuration")
        {
            return wformatString(service->ServiceDescription.QuorumLossWaitDuration.TotalSeconds());
        }
        else if (params[2] == L"StandByReplicaKeepDuration")
        {
            return wformatString(service->ServiceDescription.StandByReplicaKeepDuration.TotalSeconds());
        }
        else if (params[2] == L"UpdateVersion")
        {
            return wformatString(service->ServiceDescription.UpdateVersion);
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetRAFUState(StringCollection const & params)
{
    if (params.size() == 5)
    {
        wstring const & serviceName = params[3];
        NodeId nodeId = ParseNodeId(params[4]);
        auto failoverUnit = GetRAFailoverUnitFromParam(serviceName, nodeId);
        if (params[2] == L"Exists")
        {
            return failoverUnit ? L"true" : L"false";
        }

        if (failoverUnit)
        {
            if (params[2] == L"ReconfigStage")
            {
                return wformatString(failoverUnit->ReconfigurationStage);
            }
            else if (params[2] == L"LocalReplicaRole")
            {
                return GetReplicaRoleString(failoverUnit->LocalReplicaRole);
            }
            else if (params[2] == L"Version")
            {
                auto localReplica = failoverUnit->GetLocalReplica();
                if (!localReplica)
                {
                    return wstring();
                }

                return wformatString(localReplica->PackageVersionInstance.Version.ToString());
            }
            else if (params[2] == L"LocalReplicaDroppedReplyPending")
            {
                return wformatString(failoverUnit->LocalReplicaDroppedReplyPending);
            }
            else if (params[2] == L"LocalReplicaClosePending")
            {
                return wformatString(failoverUnit->LocalReplicaClosePending);
            }
            else if (params[2] == L"LocalReplicaDeleted")
            {
                return wformatString(failoverUnit->LocalReplicaDeleted);
            }
            else if (params[2] == L"LocalReplicaOpen")
            {
                return wformatString(failoverUnit->LocalReplicaOpen);
            }
            else if (params[2] == L"State")
            {
                return wformatString(failoverUnit->State);
            }
            else if (params[2] == L"HasServiceTypeRegistration")
            {
                return failoverUnit->ServiceTypeRegistration != nullptr ? L"true" : L"false";
            }
            else if (params[2] == L"LocalReplicaServiceDescriptionUpdatePending")
            {
                return wformatString(failoverUnit->IsServiceDescriptionUpdatePending);
            }
            else if (params[2] == L"FMMessageStage")
            {
                return wformatString(failoverUnit->FMMessageStage);
            }
            else if (params[2] == L"IsReconfiguring")
            {
                return failoverUnit->IsReconfiguring ? L"true" : L"false";
            }
            else if (params[2] == L"ServicePackageActivationId")
            {
                TestSession::FailTestIf(failoverUnit->ServiceTypeRegistration == nullptr, "ServiceTypeRegistration is null.");

                return failoverUnit->ServiceTypeRegistration->ServicePackagePublicActivationId;
            }
            else if (params[2] == L"ServicePackageActivationGuid")
            {
                TestSession::FailTestIf(failoverUnit->ServiceTypeRegistration == nullptr, "ServiceTypeRegistration is null.");

                return failoverUnit->ServiceTypeRegistration->ActivationContext.ActivationGuid.ToString();
            }
        }
        else
        {
            if (params[2] == L"State")
            {
                return wformatString(Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Closed);
            }
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetRAUpgradeState(Common::StringCollection const & params)
{
    if (params.size() == 4)
    {
        wstring identifier = params[2];
        NodeId nodeId = ParseNodeId(params[3]);

        FabricTestNodeSPtr fabricTestNode = testFederation_.GetNode(nodeId);

        // Force error if node not found
        TestSession::FailTestIfNot(fabricTestNode != nullptr, "Node {0} does not exist", nodeId);

        ReconfigurationAgentComponent::Upgrade::UpgradeStateName::Enum upgradeState = ReconfigurationAgentComponent::Upgrade::UpgradeStateName::Invalid;
        fabricTestNode->Node->GetReliabilitySubsystemUnderLock([&](ReliabilityTestHelper & reliabilityTestHelper)
        {
            auto ra = reliabilityTestHelper.GetRA();

            upgradeState = ra->GetUpgradeStateName(identifier);
        });

        return wformatString(upgradeState);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetRAReplicaState(StringCollection const & params)
{
    if (params.size() == 6)
    {
        wstring const & serviceName = params[3];
        NodeId nodeId = ParseNodeId(params[4]);
        auto failoverUnit = GetRAFailoverUnitFromParam(serviceName, nodeId);
        if(failoverUnit)
        {
            auto replicaPtr = failoverUnit->GetReplicaOnNode(ParseNodeId(params[5]));

            if (!StringUtility::CompareCaseInsensitive(params[2], L"Exists"))
            {
                return wformatString(replicaPtr != nullptr);
            }

            if (replicaPtr == nullptr)
            {
                if (params[2] == L"IsUp")
                {
                    return L"false";
                }

                return wstring();
            }

            auto const & replica = *replicaPtr;
            if (!StringUtility::CompareCaseInsensitive(params[2], L"IsUp"))
            {
                return wformatString(replica.IsUp);
            }
            else if (!StringUtility::CompareCaseInsensitive(params[2], L"State"))
            {
                return wformatString(replica.State);
            }
            else if (!StringUtility::CompareCaseInsensitive(params[2], L"MessageStage"))
            {
                return wformatString(replica.MessageStage);
            }
            else if (!StringUtility::CompareCaseInsensitive(params[2], L"ReplicaId"))
            {
                return wformatString(replica.ReplicaId);
            }
            else if (!StringUtility::CompareCaseInsensitive(params[2], L"InstanceId"))
            {
                return wformatString(replica.InstanceId);
            }
        }
    }

    return wstring();
}

wstring FabricTestDispatcher::GetReconfig(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->IsChangingConfiguration ? "true" : "false");
    }

    return wstring();
}

wstring FabricTestDispatcher::GetDataLossVersion(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->CurrentConfigurationEpoch.DataLossVersion);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetEpoch(StringCollection const& params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->CurrentConfigurationEpoch);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetQuorumLost(StringCollection const & params)
{
    UpdateQuorumOrRebuildLostFailoverUnits();
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(IsQuorumLostFU(failoverUnit->Id.Guid) || failoverUnit->IsQuorumLost() ? "true" : "false");
    }

    return L"NULL";
}

wstring FabricTestDispatcher::GetIsInRebuild(StringCollection const & params)
{
    wstring const & serviceName = params[3];

    FailoverManagerTestHelperUPtr fm;
    if (serviceName == ServiceModel::SystemServiceApplicationNameHelper::InternalFMServiceName)
    {
        fm = GetFMM();
    }
    else
    {
        fm = GetFM();
    }

    if (!fm)
    {
        return wstring();
    }

    bool serviceExistsInInBuildFailoverUnitCache = fm->InBuildFailoverUnitExistsForService(serviceName);
    return wformatString((serviceExistsInInBuildFailoverUnitCache) ? "true" : "false");
}

wstring FabricTestDispatcher::GetPartitionId(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->Id);
    }

    return wstring();
}


wstring FabricTestDispatcher::GetPartitionKind(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription.PartitionKind);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetPartitionLowKey(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription.LowKey);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetPartitionHighKey(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription.HighKey);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetPartitionName(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->FailoverUnitDescription.ConsistencyUnitDescription.PartitionName);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetTargetReplicaSetSize(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->TargetReplicaSetSize);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetMinReplicaSetSize(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->MinReplicaSetSize);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetIsOrphaned(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->IsOrphaned);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetPartitionStatus(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        return wformatString(failoverUnit->PartitionStatus);
    }

    return wstring();
}

wstring FabricTestDispatcher::GetDroppedReplicaCount(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        auto replicas = failoverUnit->GetReplicas();

        size_t count = 0;
        for(auto it = replicas.begin(); it != replicas.end(); ++it)
        {
            if (it->IsDropped)
            {
                ++count;
            }
        }

        return wformatString(count);
    }

    return wstring(L"0");
}

wstring FabricTestDispatcher::GetReplicaCount(StringCollection const & params)
{
    auto failoverUnit = GetFMFailoverUnitFromParam(params[3]);
    if (failoverUnit)
    {
        size_t count = failoverUnit->ReplicaCount;
        return wformatString(count);
    }

    return wstring(L"0");
}

std::wstring FabricTestDispatcher::GetCodePackageId(Common::StringCollection const & params)
{
    if (params.size() >= 4)
    {
        ServiceModel::ApplicationIdentifier appId = ApplicationMap.GetAppId(params[1]);
        wstring const & servicePackageName = params[2];
        wstring const & codePackageName = params[3];

        ServiceModel::ServicePackageActivationContext activationContext;
        wstring servicePackagePublicActivationId;
        
        if (params.size() == 5)
        {
            TestSession::WriteError(TraceSource, "GetCodePackageId(): Invalid number of params specified: 5");
        }
        
        if (params.size() == 6)
        {
            auto error = ServiceModel::ServicePackageActivationContext::FromString(params[4], activationContext);
            if (!error.IsSuccess())
            {
                TestSession::WriteError(
                    TraceSource,
                    "GetCodePackageId(): Invalid ServicePackageActivationGuid specified: {0}",
                    params[4]);

                return wstring();
            }

            servicePackagePublicActivationId = params[5];

            if (activationContext.IsExclusive && servicePackagePublicActivationId.empty())
            {
                TestSession::WriteError(
                    TraceSource,
                    "GetCodePackageId(): Empty ServicePackagePublicActivationId specified for exclusive ServicePackageActivationContext.");
            }
        }

        ServicePackageInstanceIdentifier servicePackageInstanceId(
            ServiceModel::ServicePackageIdentifier(appId, servicePackageName),
            activationContext,
            servicePackagePublicActivationId);

        CodePackageInstanceIdentifier codePackageInstanceIdentifier(servicePackageInstanceId, codePackageName);

        return codePackageInstanceIdentifier.ToString();
    }

    return wstring();
}

//dcc8fc37-9472-4afc-b877-6138418a6033
static const GUID CLSID_ComTestServiceNotificationEventHandler =
    {0xdcc8fc37,0x9472,0x4afc,{0xb8,0x77,0x61,0x38,0x41,0x8a,0x60,0x33}};

class FabricTestDispatcher::ComTestServiceNotificationEventHandler
    : public ComUnknownBase
    , public IFabricServiceNotificationEventHandler
{
    BEGIN_COM_INTERFACE_LIST(ComTestServiceNotificationEventHandler)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceNotificationEventHandler)
        COM_INTERFACE_ITEM(IID_IFabricServiceNotificationEventHandler, IFabricServiceNotificationEventHandler)
        COM_INTERFACE_ITEM(CLSID_ComTestServiceNotificationEventHandler, ComTestServiceNotificationEventHandler)
    END_COM_INTERFACE_LIST()

public:
    ComTestServiceNotificationEventHandler()
        : expectedNames_()
        , pendingNames_()
        , pendingPrimaries_()
        , allowNullPartitionInfo_(false)
        , notificationNamesLock_()
        , notificationEvent_()
    {
    }

    HRESULT OnNotification(IFabricServiceNotification * notification)
    {
        FABRIC_SERVICE_NOTIFICATION const * ptr = notification->get_Notification();

        wstring endpoints;
        bool hasPrimary = false;

        for (ULONG ix=0; ix<ptr->EndpointCount; ++ix)
        {
            if (!endpoints.empty()) { endpoints.append(L"\n\r"); }

            FABRIC_RESOLVED_SERVICE_ENDPOINT const & endpoint = ptr->Endpoints[ix];

            endpoints.append(wformatString(
                "address={0} role={1}",
                wstring(endpoint.Address),
                static_cast<int>(endpoint.Role)));

            if (endpoint.Role == FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY)
            {
                hasPrimary = true;
            }
        }

        wstring partitionInfo;
        if (ptr->PartitionInfo != NULL)
        {
            switch(ptr->PartitionInfo->Kind)
            {
                case FABRIC_SERVICE_PARTITION_KIND_INVALID:
                    partitionInfo.append(L"invalid");
                    break;

                case FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
                {
                    auto casted = reinterpret_cast<FABRIC_SINGLETON_PARTITION_INFORMATION*>(ptr->PartitionInfo->Value);

                    partitionInfo.append(wformatString("singleton({0})", Guid(casted->Id)));
                    break;
                }

                case FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
                {
                    auto casted = reinterpret_cast<FABRIC_INT64_RANGE_PARTITION_INFORMATION*>(ptr->PartitionInfo->Value);

                    partitionInfo.append(wformatString(
                        "uniform({0})[{1}, {2}]",
                        Guid(casted->Id),
                        casted->LowKey,
                        casted->HighKey));
                    break;
                }

                case FABRIC_SERVICE_PARTITION_KIND_NAMED:
                {
                    auto casted = reinterpret_cast<FABRIC_NAMED_PARTITION_INFORMATION*>(ptr->PartitionInfo->Value);

                    partitionInfo.append(wformatString(
                        "named({0})[{1}]",
                        Guid(casted->Id),
                        wstring(casted->Name)));
                    break;
                }

                default:
                    partitionInfo.append(L"unknown");
                    break;
            }
        }
        else
        {
            partitionInfo.append(L"null");
        }

        wstring name(ptr->ServiceName);

        ComPointer<IFabricServiceEndpointsVersion> notificationVersion;
        auto hr = notification->GetVersion(notificationVersion.InitializationAddress());

        TestSession::FailTestIfNot(
            SUCCEEDED(hr),
            "GetVersion() failed: hr={0}",
            hr);

        LONG compareResult = 0;
        if (lastNotificationVersion_)
        {
            hr = lastNotificationVersion_->Compare(notificationVersion.GetRawPointer(), &compareResult);

            TestSession::FailTestIfNot(
                SUCCEEDED(hr),
                "Compare() failed: hr={0}",
                hr);

            lastNotificationVersion_.Swap(notificationVersion);
        }

        TestSession::WriteInfo(
            TraceSource,
            "Service Notification: uri={0} partition={1} count={2} info={3} compare={4} endpoints=({5})",
            name,
            Guid(ptr->PartitionId),
            ptr->EndpointCount,
            partitionInfo,
            compareResult,
            endpoints);

        if (!allowNullPartitionInfo_)
        {
            TestSession::FailTestIf(
                ptr->EndpointCount > 0 && ptr->PartitionInfo == NULL,
                "NULL PartitionInfo for EndpointCount={0}",
                ptr->EndpointCount);
        }

        this->FinishNotification(name, hasPrimary);

        return S_OK;
    }

    void PrepareWaitForNotifications(vector<wstring> const & names, vector<wstring> const & primaries, bool const allowNullPartitionInfo)
    {
        AcquireWriteLock lock(notificationNamesLock_);

        expectedNames_.clear();
        pendingNames_.clear();
        pendingPrimaries_.clear();

        for (auto const & name : names)
        {
            expectedNames_.insert(name);
            pendingNames_.insert(name);
        }

        for (auto const & primary : primaries)
        {
            expectedNames_.insert(primary);
            pendingPrimaries_.insert(primary);
        }

        allowNullPartitionInfo_ = allowNullPartitionInfo;

        notificationEvent_.Reset();
    }

    void WaitForNotifications(TimeSpan const timeout)
    {
        auto success = notificationEvent_.WaitOne(timeout);

        AcquireReadLock lock(notificationNamesLock_);

        TestSession::FailTestIfNot(
            success,
            "Timed out waiting for notifications: remaining={0}",
            pendingNames_);
    }

private:

    void FinishNotification(wstring const & name, bool hasPrimary)
    {
        AcquireWriteLock lock(notificationNamesLock_);

        auto findIt = expectedNames_.find(name);

        TestSession::FailTestIf(
            findIt == expectedNames_.end(),
            "Unexpected notification: name={0} expecting={1}",
            name,
            expectedNames_);

        pendingNames_.erase(name);

        if (hasPrimary)
        {
            pendingPrimaries_.erase(name);
        }

        if (pendingNames_.empty() && pendingPrimaries_.empty())
        {
            notificationEvent_.Set();
        }
    }

    set<wstring> expectedNames_;
    set<wstring> pendingNames_;
    set<wstring> pendingPrimaries_;
    bool allowNullPartitionInfo_;
    RwLock notificationNamesLock_;
    ManualResetEvent notificationEvent_;

    ComPointer<IFabricServiceEndpointsVersion> lastNotificationVersion_;
};

bool FabricTestDispatcher::CreateServiceNotificationClient(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    vector<wstring> nodeIdStrings;
    parser.GetVector(L"nodes", nodeIdStrings, L"");

    wstring clientName;
    parser.TryGetString(L"clientname", clientName, L"ServiceNotificationClient");

    TestSession::FailTestIf(nodeIdStrings.empty() || clientName.empty(), "nodes and clientname must be non-empty");

    auto comPtr = make_com<ComTestServiceNotificationEventHandler, IFabricServiceNotificationEventHandler>();
    auto comProxy = Api::WrapperFactory::create_rooted_com_proxy(comPtr.GetRawPointer());

    CreateServiceNotificationClientOnNodes(nodeIdStrings, clientName, comProxy);

    auto result = serviceNotificationEventHandlers_.insert(make_pair(clientName, comPtr));
    TestSession::FailTestIfNot(result.second, "ComTestServiceNotificationEventHandler already exists for client {0}", clientName);

    return true;
}

bool FabricTestDispatcher::SetServiceNotificationWait(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    vector<wstring> names;
    parser.GetVector(L"names", names, L"");

    vector<wstring> primaries;
    parser.GetVector(L"primaries", primaries, L"");

    wstring clientName;
    parser.TryGetString(L"clientname", clientName, L"ServiceNotificationClient");

    bool allowNullPartitionInfo = parser.GetBool(L"allowNullPartitionInfo");

    TestSession::FailTestIf(names.empty() || clientName.empty(), "names and clientname must be non-empty");

    auto findIt = serviceNotificationEventHandlers_.find(clientName);
    TestSession::FailTestIf(
        findIt == serviceNotificationEventHandlers_.end(),
        "ComTestServiceNotificationEventHandler does not exist for client {0}",
        clientName);

    ComPointer<ComTestServiceNotificationEventHandler> comPtr;
    auto hr = findIt->second->QueryInterface(CLSID_ComTestServiceNotificationEventHandler, comPtr.VoidInitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "Could not QI for ComTestServiceNotificationEventHandler: hr={0}", hr);

    comPtr->PrepareWaitForNotifications(names, primaries, allowNullPartitionInfo);

    return true;
}

bool FabricTestDispatcher::ServiceNotificationWait(StringCollection const & params)
{
    CommandLineParser parser(params, 0);

    int timeout;
    parser.TryGetInt(L"timeout", timeout, 10);

    wstring clientName;
    parser.TryGetString(L"clientname", clientName, L"ServiceNotificationClient");

    TestSession::FailTestIf(clientName.empty(), "names and clientname must be non-empty");

    auto findIt = serviceNotificationEventHandlers_.find(clientName);
    TestSession::FailTestIf(
        findIt == serviceNotificationEventHandlers_.end(),
        "ComTestServiceNotificationEventHandler does not exist for client {0}",
        clientName);

    ComPointer<ComTestServiceNotificationEventHandler> comPtr;
    auto hr = findIt->second->QueryInterface(CLSID_ComTestServiceNotificationEventHandler, comPtr.VoidInitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "Could not QI for ComTestServiceNotificationEventHandler: hr={0}", hr);

    comPtr->WaitForNotifications(TimeSpan::FromSeconds(timeout));

    return true;
}

void FabricTestDispatcher::CreateServiceNotificationClientOnNodes(
    vector<wstring> const & nodeIdStrings,
    wstring const & clientName,
    IServiceNotificationEventHandlerPtr const & handler)
{
    vector<wstring> gatewayAddresses;

    for (auto const & nodeIdString : nodeIdStrings)
    {
        auto config = clientsTracker_.GetFabricNodeConfig(nodeIdString);

        gatewayAddresses.push_back(config->ClientConnectionAddress);
    }

    auto client = make_shared<FabricClientImpl>(move(gatewayAddresses), handler);
    auto settings = client->GetSettings();
    settings.SetClientFriendlyName(clientName);
    client->SetSettings(move(settings));
    client->SetSecurity(SecuritySettings(FABRICSESSION.FabricDispatcher.FabricClientSecurity));

    IClientFactoryPtr clientFactory;
    auto error = ClientFactory::CreateClientFactory(client, clientFactory);
    TestSession::FailTestIfNot(error.IsSuccess(), "CreateClientFactory failed: error = {0}", error);

    ComPointer<ComFabricClient> comClient;
    auto hr = ComFabricClient::CreateComFabricClient(clientFactory, comClient);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "CreateComFabricClient failed: hr = {0}", hr);

    ComPointer<IFabricServiceManagementClient> serviceClient;
    hr = comClient->QueryInterface(IID_IFabricServiceManagementClient, serviceClient.VoidInitializationAddress());
    TestSession::FailTestIfNot(SUCCEEDED(hr), "QueryInterface for IID_IFabricServiceManagementClient failed: hr = {0}", hr);

    bool added = clientManager_.AddClient(clientName, move(serviceClient));
    TestSession::FailTestIfNot(added, "Client {0} already exists", clientName);
}

void FabricTestDispatcher::CreateFabricClientOnNode(
    std::wstring const & nodeIdString,
    __out ComPointer<IFabricServiceManagementClient> & serviceClient)
{
    auto factoryPtr = clientsTracker_.CreateClientFactoryOnNode(nodeIdString);

    ComPointer<ComFabricClient> result;
    auto hr = ComFabricClient::CreateComFabricClient(factoryPtr, result);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "CreateComFabricClient failed: hr = {0} ({1})", hr, ErrorCode::FromHResult(hr));

    result->QueryInterface(IID_IFabricServiceManagementClient, (void**)serviceClient.InitializationAddress());
}

void FabricTestDispatcher::CreateFabricClientOnNode(
    std::wstring const & nodeIdString,
    __out ComPointer<IFabricServiceManagementClient5> & serviceClient)
{
    auto factoryPtr = clientsTracker_.CreateClientFactoryOnNode(nodeIdString);

    ComPointer<ComFabricClient> result;
    auto hr = ComFabricClient::CreateComFabricClient(factoryPtr, result);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "CreateComFabricClient failed: hr = {0} ({1})", hr, ErrorCode::FromHResult(hr));

    result->QueryInterface(IID_IFabricServiceManagementClient5, (void**)serviceClient.InitializationAddress());
}

void FabricTestDispatcher::CreateFabricClientAtAddresses(
    StringCollection const & addresses,
    __out ComPointer<IFabricServiceManagementClient> & serviceClient)
{
    TestSession::FailTestIf(addresses.empty(), "addresses cannot be empty");

    vector<LPCWSTR> connectionStrings;
    for (auto iter = addresses.begin(); iter != addresses.end(); ++iter)
    {
        connectionStrings.push_back(iter->c_str());
    }

    IClientFactoryPtr factoryPtr;
    auto error = Client::ClientFactory::CreateClientFactory(
        static_cast<USHORT>(connectionStrings.size()),
        connectionStrings.data(),
        factoryPtr);
    TestSession::FailTestIfNot(error.IsSuccess(), "Could not create fabric client factory to {0}. error = {1}", connectionStrings[0], error);

    ComPointer<ComFabricClient> result;
    auto hr = ComFabricClient::CreateComFabricClient(factoryPtr, result);
    TestSession::FailTestIfNot(!FAILED(hr), "CreateComFabricClient failed", error);

    result->QueryInterface(IID_IFabricServiceManagementClient, (void**)serviceClient.InitializationAddress());
}

void FabricTestDispatcher::CreateNativeImageStoreClient(
    __out ComPointer<IFabricNativeImageStoreClient> & client)
{
    auto factoryPtr = this->CreateClientFactory();

    INativeImageStoreClientPtr implPtr;

    Management::ImageStore::NativeImageStore::CreateNativeImageStoreClient(false, L"", factoryPtr, implPtr);

    client = ComNativeImageStoreClient::CreateComNativeImageStoreClient(implPtr);
}

IClientFactoryPtr FabricTestDispatcher::CreateClientFactory()
{
    FabricTestNodeSPtr selectedNode;
    auto nodes = testFederation_.GetNodes();
    for (auto const & node : nodes)
    {
        if(node->IsActivated)
        {
            selectedNode = node;
        }
    }

    if (!selectedNode)
    {
        TestSession::FailTest("None of the nodes are activated");
    }

    shared_ptr<FabricNodeWrapper> fabricNodeWrapper;
    TestSession::FailTestIfNot(selectedNode->TryGetFabricNodeWrapper(fabricNodeWrapper), "TryGetFabricNodeWrapper should succeed");

    IClientFactoryPtr factoryPtr;
    auto error = Client::ClientFactory::CreateLocalClientFactory(
        fabricNodeWrapper->Config,
        factoryPtr);
    TestSession::FailTestIfNot(error.IsSuccess(), "Could not create local fabric client factory error = {1}", error);

    return factoryPtr;
}

template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricServiceManagementClient4>(wstring const &, REFIID, __out ComPointer<IFabricServiceManagementClient4> &);
template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricServiceManagementClient>(wstring const &, REFIID, __out ComPointer<IFabricServiceManagementClient> &);
template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricPropertyManagementClient>(wstring const &, REFIID, __out ComPointer<IFabricPropertyManagementClient> &);
template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricServiceGroupManagementClient>(wstring const &, REFIID, __out ComPointer<IFabricServiceGroupManagementClient> &);
template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricClusterManagementClient>(wstring const &, REFIID, __out ComPointer<IFabricClusterManagementClient> &);
template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricQueryClient>(wstring const &, REFIID, __out ComPointer<IFabricQueryClient> &);
template bool FabricTestDispatcher::TryGetNamedFabricClient<IFabricApplicationManagementClient>(wstring const &, REFIID, __out ComPointer<IFabricApplicationManagementClient> &);

template <class TInterface>
bool FabricTestDispatcher::TryGetNamedFabricClient(wstring const & clientName, REFIID riid, __out ComPointer<TInterface> & result)
{
    if (!clientName.empty())
    {
        ServiceLocationChangeClientSPtr client;
        if (clientManager_.FindClient(clientName, client))
        {
            HRESULT hr = client->Client->QueryInterface(riid, result.VoidInitializationAddress());
            TestSession::FailTestIfNot(SUCCEEDED(hr), "QI failed for named client '{0}'", clientName);

            return true;
        }
    }

    return false;
}

bool FabricTestDispatcher::IsRebuildLostFailoverUnit(Guid partitionId)
{
    auto it = find(rebuildLostFailoverUnits_.begin(), rebuildLostFailoverUnits_.end(), partitionId);
    return it != rebuildLostFailoverUnits_.end();
}

bool FabricTestDispatcher::IsSeedNode(NodeId nodeId)
{
    auto votes = FederationConfig::GetConfig().Votes;
    auto it = votes.find(nodeId);
    return it != votes.end() && it->Type == Federation::Constants::SeedNodeVoteType;
}

void FabricTestDispatcher::SetDefaultCertIssuers()
{
    ClientSecurityHelper::SetDefaultCertIssuers();
}

void FabricTestDispatcher::ClearDefaultCertIssuers()
{
    ClientSecurityHelper::ClearDefaultCertIssuers();
}

bool FabricTestDispatcher::SetEseOnly()
{
    isSkipForTStoreEnabled_ = true;

    return true;
}

bool FabricTestDispatcher::ClearEseOnly()
{
    isSkipForTStoreEnabled_ = false;

    return true;
}

/// <summary>
/// Copies a directory to another directory.
/// If the destination directory doesn't exist, it is created.
/// To overwrite/merge contents to an existing destination directory, use the optional overwrite paramater
/// </summary>
/// <param name="params">The parameters which are - source dir, destination dir and an optional overwrite destination dir flag (true | false).</param>
/// <returns>Returns true if the command was executed successfully. Returns false if the input params collection is invalid.</returns>
bool FabricTestDispatcher::Xcopy(Common::StringCollection const & params)
{
    if (params.size() < 2 || params.size() > 3)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::XcopyCommand);
        return false;
    }

    bool overwrite = params.size() == 3 ? IsTrue(params[2]) : false;

    ErrorCode error = Directory::Copy(params[0], params[1], overwrite);

    return error.IsSuccess();
}

DWORD FabricTestDispatcher::GetComTimeout(TimeSpan cTimeout)
{
    DWORD comTimeout;
    HRESULT hr = Int64ToDWord(cTimeout.TotalPositiveMilliseconds(), &comTimeout);
    TestSession::FailTestIfNot(SUCCEEDED(hr), "Could not convert timeout from int64 to DWORD. hr = {0}", hr);

    return comTimeout;
}


bool FabricTestDispatcher::AddLeaseBehavior(Common::StringCollection const & params)
{
    if (params.size() <4 && params.size()>5)
    {
        return false;
    }

    wstring localAddress = AddressHelper::BuildAddress(params[0]);
    wstring remoteAddress = AddressHelper::BuildAddress(params[1]);
    wstring messageType = params[2];

    bool fromAny = (localAddress[localAddress.size() - 1] == L'*');
    bool toAny = (remoteAddress[remoteAddress.size() - 1] == L'*');

    wstring alias;
    if (params.size() == 4){
        alias = params[3];
    }
    LEASE_BLOCKING_ACTION_TYPE messageTypeEnum;

    if (messageType == L"request"){
        messageTypeEnum = LEASE_ESTABLISH_ACTION;
    }
    else if (messageType == L"response"){
        messageTypeEnum = LEASE_ESTABLISH_RESPONSE;
    }
    else if (messageType == L"indirect"){
        messageTypeEnum = LEASE_INDIRECT;
    }
    else if (messageType == L"ping_response"){
        messageTypeEnum = LEASE_PING_RESPONSE;
    }
    else if (messageType == L"ping_request"){
        messageTypeEnum = LEASE_PING_REQUEST;
    }
    else{
        messageTypeEnum = LEASE_BLOCKING_ALL;
    }

    TRANSPORT_LISTEN_ENDPOINT ListenEndPointLocal;
    TRANSPORT_LISTEN_ENDPOINT ListenEndPointRemote;

    if (!fromAny && !LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, localAddress, ListenEndPointLocal))
    {
        return false;
    }

    if (!toAny && !LeaseWrapper::LeaseAgent::InitializeListenEndpoint(nullptr, remoteAddress, ListenEndPointRemote))
    {
        return false;
    }
    #ifndef PLATFORM_UNIX

    addLeaseBehavior(&ListenEndPointRemote, toAny, &ListenEndPointLocal, fromAny, messageTypeEnum, alias);
    
    #endif

    return true;
}


bool FabricTestDispatcher::RemoveLeaseBehavior(Common::StringCollection const & params)
{
    if (params.size() > 1)
    {
        return false;
    }

    wstring alias;
    if (params.size() == 1){
        alias = params[0];
    }

    #ifndef PLATFORM_UNIX

    ::removeLeaseBehavior(alias);
    
    #endif
    
    return true;
}

bool FabricTestDispatcher::AddDiskDriveFolders(Common::StringCollection const & params)
{
    if (params.size() < 1)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::AddDiskDriveFoldersCommand);
        return false;
    }
    CommandLineParser parser(params, 0);

    vector<wstring> directories;
    parser.GetVector(L"dirname", directories, L"");

    TestSession::FailTestIf(directories.empty(), "Directory Names cannot be empty");

    wstring currentDirectory = Directory::GetCurrentDirectory();

    ConfigSettings configSettings;
    ConfigSection section;
    ConfigParameter parameter;
    section.Name = L"DiskDrives";

    std::map<std::wstring, std::wstring> diskDrives;
    for (auto dirName : directories)
    {
        parameter.Name = dirName;
        parameter.Value = Path::Combine(currentDirectory, dirName);

        section.Parameters[parameter.Name] = parameter;
     }

    configSettings.Sections[section.Name] = section;

    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        shared_ptr<FabricNodeWrapper> nodeWrapper;
        if (!node->TryGetFabricNodeWrapper(nodeWrapper))
        {
            TestSession::WriteWarning(
                TraceSource,
                "Cannot get node wrapper for ID {0}.",
                node->get_Id());
            return false;
        }

        shared_ptr<ConfigSettingsConfigStore> configStore = make_shared<ConfigSettingsConfigStore>(configSettings);
        FabricNodeConfigSPtr fabricNodeConfigSPtr = make_shared<FabricNodeConfig>(configStore);
        nodeWrapper->GetHostingSubsystem().Test_SetFabricNodeConfig(move(fabricNodeConfigSPtr));
    }

    return true;
}

bool FabricTestDispatcher::VerifyDeletedDiskDriveFolders()
{
    for (FabricTestNodeSPtr const & node : testFederation_.GetNodes())
    {
        shared_ptr<FabricNodeWrapper> nodeWrapper;
        if (!node->TryGetFabricNodeWrapper(nodeWrapper))
        {
            TestSession::WriteWarning(
                TraceSource,
                "Cannot get node wrapper for ID {0}.",
                node->get_Id());
            return false;
        }

        std::map<wstring, wstring> diskDrives = nodeWrapper->GetHostingSubsystem().FabricNodeConfigObj.LogicalApplicationDirectories;

        for (auto iter = diskDrives.begin(); iter != diskDrives.end(); ++iter)
        {
            std::vector<std::wstring> nodeDirectories = Directory::GetSubDirectories(iter->second);
            for (auto nodeDir : nodeDirectories)
            {
                wstring directoryToSearch = Path::Combine(iter->second, nodeDir);
                TestSession::FailTestIf(!Directory::GetSubDirectories(directoryToSearch).empty(), "Node Directory Path:{0} should be empty", nodeDir);
            }
        }
    }
    return true;
}

bool FabricTestDispatcher::RequestCheckpoint(Common::StringCollection const & params)
{
    if (params.size() < 2)
    {
        FABRICSESSION.PrintHelp(FabricTestCommands::RequestCheckpoint);
        return false;
    }

    wstring serviceName = params[0];
    NodeId nodeId = ParseNodeId(params[1]);
    

    ErrorCode errorCode;

    FindAndInvoke(
        nodeId,
        serviceName,
        [&](ITestStoreServiceSPtr const & storeService) { errorCode = storeService->RequestCheckpoint(); },
        nullptr);

    return errorCode.IsSuccess();
}
