// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Client;
using namespace Transport;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Reliability::LoadBalancingComponent;
using namespace Management::NetworkInventoryManager;
using namespace Federation;
using namespace std;
using namespace Store;

Global<FailoverManagerComponent::EventSource> FailoverManager::FMEventSource = make_global<FailoverManagerComponent::EventSource>(Common::TraceTaskCodes::FM);
Global<FailoverManagerComponent::MessageEventSource> FailoverManager::FMMessageEventSource = make_global<FailoverManagerComponent::MessageEventSource>(Common::TraceTaskCodes::FM);
Global<FailoverManagerComponent::FTEventSource> FailoverManager::FMFTEventSource = make_global<FailoverManagerComponent::FTEventSource>(Common::TraceTaskCodes::FM);
Global<FailoverManagerComponent::ServiceEventSource> FailoverManager::FMServiceEventSource = make_global<FailoverManagerComponent::ServiceEventSource>(Common::TraceTaskCodes::FM);
Global<FailoverManagerComponent::NodeEventSource> FailoverManager::FMNodeEventSource = make_global<FailoverManagerComponent::NodeEventSource>(Common::TraceTaskCodes::FM);

Global<FailoverManagerComponent::EventSource> FailoverManager::FMMEventSource = make_global<FailoverManagerComponent::EventSource>(Common::TraceTaskCodes::FMM);
Global<FailoverManagerComponent::MessageEventSource> FailoverManager::FMMMessageEventSource = make_global<FailoverManagerComponent::MessageEventSource>(Common::TraceTaskCodes::FMM);
Global<FailoverManagerComponent::FTEventSource> FailoverManager::FMMFTEventSource = make_global<FailoverManagerComponent::FTEventSource>(Common::TraceTaskCodes::FMM);
Global<FailoverManagerComponent::ServiceEventSource> FailoverManager::FMMServiceEventSource = make_global<FailoverManagerComponent::ServiceEventSource>(Common::TraceTaskCodes::FMM);
Global<FailoverManagerComponent::NodeEventSource> FailoverManager::FMMNodeEventSource = make_global<FailoverManagerComponent::NodeEventSource>(Common::TraceTaskCodes::FMM);

namespace
{
    StringLiteral const TraceLifeCycle("LifeCycle");
    StringLiteral const TraceFailure("Failure");
    StringLiteral const TraceReceive("Receive");
}

StringLiteral const FailoverManager::TraceFabricUpgrade("FabricUpgrade");
StringLiteral const FailoverManager::TraceApplicationUpgrade("AppUpgrade");
StringLiteral const FailoverManager::TraceDeactivateNode("DeactivateNode");

IFailoverManagerSPtr Reliability::FailoverManagerComponent::FMMFactory(FailoverManagerConstructorParameters & parameters)
{
    return FailoverManager::CreateFMM(
        move(parameters.Federation),
        parameters.HealthClient,
        parameters.NodeConfig);
}

FailoverManagerSPtr FailoverManager::CreateFMM(
    FederationSubsystemSPtr && federation,
    HealthReportingComponentSPtr const & healthClient,
    FabricNodeConfigSPtr const& nodeConfig)
{
    unique_ptr<Store::IReplicatedStore> replicatedStore = make_unique<FauxLocalStore>();
    auto fmStore = make_unique<FailoverManagerStore>(move(replicatedStore));
    wstring fmId = federation->Instance.ToString() + L":" + StringUtility::ToWString(SequenceNumber::GetNext());
    ComPointer<IFabricStatefulServicePartition> servicePartition;
    return make_shared<FailoverManager>(move(federation), healthClient, Api::IServiceManagementClientPtr(), nodeConfig, true, move(fmStore), servicePartition, fmId, fmId);
}

FailoverManagerSPtr FailoverManager::CreateFM(
    FederationSubsystemSPtr && federation,
    HealthReportingComponentSPtr const & healthClient,
    Api::IServiceManagementClientPtr serviceManagementClient,
    Api::IClientFactoryPtr const & clientFactory,
    FabricNodeConfigSPtr const& nodeConfig,
    FailoverManagerStoreUPtr && fmStore,
    ComPointer<IFabricStatefulServicePartition> const& servicePartition,
    Guid const& partitionId,
    int64 replicaId)
{
    wstring fmId = federation->Instance.ToString() + L":" + StringUtility::ToWString(SequenceNumber::GetNext());
    wstring performanceCounterInstanceName = partitionId.ToString() + L":" + wformatString(replicaId) + L":" + wformatString(Common::SequenceNumber::GetNext());
    auto fm = make_shared<FailoverManager>(
        move(federation),
        healthClient,
        serviceManagementClient,
        nodeConfig,
        false, // IsMaster
        move(fmStore),
        servicePartition,
        fmId,
        performanceCounterInstanceName);
    fm->ClientFactory = clientFactory;

    return fm;
}

FailoverManager::FailoverManager(
    FederationSubsystemSPtr && federation,
    HealthReportingComponentSPtr const & healthClient,
    Api::IServiceManagementClientPtr serviceManagementClient,
    FabricNodeConfigSPtr const& nodeConfig,
    bool isMaster,
    FailoverManagerStoreUPtr && fmStore,
    ComPointer<IFabricStatefulServicePartition> const& servicePartition,
    wstring const& fmId,
    wstring const& performanceCounterInstanceName)
    : WriteError(isMaster ? Common::TraceTaskCodes::FMM : Common::TraceTaskCodes::FM, LogLevel::Error),
    WriteWarning(isMaster ? Common::TraceTaskCodes::FMM : Common::TraceTaskCodes::FM, LogLevel::Warning),
    WriteInfo(isMaster ? Common::TraceTaskCodes::FMM : Common::TraceTaskCodes::FM, LogLevel::Info),
    WriteNoise(isMaster ? Common::TraceTaskCodes::FMM : Common::TraceTaskCodes::FM, LogLevel::Noise),
    FailoverManagerComponent::IFailoverManager(fmId),
    federation_(move(federation)),
    healthClient_(healthClient),
    serviceManagementClient_(serviceManagementClient),
    nodeConfig_(nodeConfig),
    isMaster_(isMaster),
    messageProcessingActor_(isMaster_ ? Actor::FMM : Actor::FM),
    isActive_(false),
    isReady_(false),
    readyTime_(DateTime::MaxValue),
    fmReadyEvent_(make_shared<Common::Event>()),
    fmFailedEvent_(make_shared<Common::Event>()),
    backgroundManager_(),
    fmStore_(move(fmStore)),
    servicePartition_(servicePartition),
    serviceCache_(),
    failoverUnitCache_(),
    nodeCache_(),
    loadCache_(),
    inBuildFailoverUnitCache_(),
    plb_(),
    messageProcessor_(),
    lockObject_(),
    storeOpenTimeoutHelper_(TimeSpan::MaxValue),
    fmQueryHelper_(),
    queueFullThrottle_(FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessagesEntry),
    healthReportFactory_(federation_->IdString, isMaster, nodeConfig->StateTraceIntervalEntry)
{
    ASSERT_IF(fmStore_ == nullptr, "FM Store cannot be null.")    
    WriteInfo(TraceLifeCycle, "{0} created", Id);
    failoverUnitCountersSPtr_ = FailoverUnitCounters::CreateInstance(performanceCounterInstanceName);
}

FailoverManager::~FailoverManager()
{
    WriteInfo(TraceLifeCycle, "{0} destructed", Id);
}

FailoverManagerComponent::EventSource const& FailoverManager::get_Events() const
{
    return isMaster_ ? *FMMEventSource : *FMEventSource;
}

FailoverManagerComponent::MessageEventSource const& FailoverManager::get_MessageEvents() const
{
    return isMaster_ ? *FMMMessageEventSource : *FMMessageEventSource;
}

FailoverManagerComponent::FTEventSource const& FailoverManager::get_FTEvents() const
{
    return isMaster_ ? *FMMFTEventSource : *FMFTEventSource;
}

FailoverManagerComponent::ServiceEventSource const& FailoverManager::get_ServiceEvents() const
{
    return isMaster_ ? *FMMServiceEventSource : *FMServiceEventSource;
}

FailoverManagerComponent::NodeEventSource const& FailoverManager::get_NodeEvents() const
{
    return isMaster_ ? *FMMNodeEventSource : *FMNodeEventSource;
}

void FailoverManager::Open(EventHandler const & readyEvent, EventHandler const & failureEvent)
{
    AcquireExclusiveLock lock(lockObject_);

    ASSERT_IF(isActive_, "FM {0} already active", Id);

    isActive_ = true;
    activateTime_ = Stopwatch::Now();
    Events.Activated(Id, federation_->Instance.Id, federation_->Instance.InstanceId);

    // LFUMUpload message needs to be processed before FM is ready so common queue needs
    // to be created earlier.

    commonQueue_ = make_unique<CommonJobQueue>(
        L"FMCommon." + Id,
        *this, // Root
        false, // ForceEnqueue
        FailoverConfig::GetConfig().CommonQueueThreadCount,
        JobQueuePerfCounters::CreateInstance(L"FMCommon", Id),
        FailoverConfig::GetConfig().CommonQueueSize);
    RegisterEvents(readyEvent, failureEvent);

    storeOpenTimeoutHelper_.SetRemainingTime(FailoverConfig::GetConfig().StoreOpenTimeout);

    StartLoading(false);
}

void FailoverManager::StartLoading(bool isRetry)
{
    auto thisRoot = this->CreateComponentRoot();

    if (!isRetry)
    {
        Threadpool::Post([this, thisRoot]() mutable { Load(); });
    }
    else
    {
        Threadpool::Post(
            [this, thisRoot]() mutable
            {
                Load();
            },
            FailoverConfig::GetConfig().StoreRetryInterval);
    }
}

void FailoverManager::Load()
{
    AcquireExclusiveLock lock(lockObject_);

    if (!IsActive)
    {
        return;
    }

    if (!storeOpenTimeoutHelper_.IsExpired)
    {
        vector<NodeInfoSPtr> nodeInfos;
        vector<FailoverUnitUPtr> failoverUnits;
        vector<ApplicationInfoSPtr> applications;
        vector<ServiceTypeSPtr> serviceTypes;
        vector<ServiceInfoSPtr> serviceInfos;
        vector<LoadInfoSPtr> loadInfos;
        vector<InBuildFailoverUnitUPtr> inBuildFailoverUnits;
        FabricVersionInstance currentVersionInstance;
        FabricUpgradeUPtr fabricUpgrade;

        ErrorCode error;

        if (!nodeCache_)
        {
            error = fmStore_->LoadAll(nodeInfos);
        }

        if (error.IsSuccess() && !failoverUnitCache_)
        {
            WriteInfo(TraceLifeCycle, "{0} nodes loaded", nodeInfos.size());
            error = fmStore_->LoadAll(failoverUnits);
        }

        if (error.IsSuccess() && !serviceCache_)
        {
            WriteInfo(TraceLifeCycle, "{0} FTs loaded", failoverUnits.size());
            error = fmStore_->LoadAll(applications);
        }

        if (error.IsSuccess() && !serviceCache_)
        {
            WriteInfo(TraceLifeCycle, "{0} applications loaded", applications.size());
            error = fmStore_->LoadAll(serviceTypes);
        }

        if (error.IsSuccess() && !serviceCache_)
        {
            WriteInfo(TraceLifeCycle, "{0} serviceTypes loaded", serviceTypes.size());
            error = fmStore_->LoadAll(serviceInfos);
        }

        if (error.IsSuccess() && !loadCache_)
        {
            WriteInfo(TraceLifeCycle, "{0} serviceInfos loaded", serviceInfos.size());
            error = fmStore_->LoadAll(loadInfos);
            if (error.IsSuccess() && !loadInfos.empty())
            {
                AdjustTimestamps(loadInfos);
            }
        }

        if (error.IsSuccess() && !inBuildFailoverUnitCache_)
        {
            WriteInfo(TraceLifeCycle, "{0} loadInfos loaded", loadInfos.size());
            error = fmStore_->LoadAll(inBuildFailoverUnits);
        }

        if (error.IsSuccess())
        {
            WriteInfo(TraceLifeCycle, "{0} inBuildFailoverUnits loaded", inBuildFailoverUnits.size());

            error = fmStore_->GetFabricVersionInstance(currentVersionInstance);
            if (error.IsError(ErrorCodeValue::FMStoreKeyNotFound))
            {
                currentVersionInstance = FabricVersionInstance::Invalid;
                error = ErrorCode::Success();
            }
        }

        if (error.IsSuccess() && !fabricUpgrade)
        {
            WriteInfo(TraceLifeCycle, "Fabric version instance: {0}", currentVersionInstance);

            error = fmStore_->GetFabricUpgrade(fabricUpgrade);
            if (error.IsError(ErrorCodeValue::FMStoreKeyNotFound))
            {
                error = ErrorCode::Success();
            }
        }

        map<wstring, wstring> keyValues;
        if (error.IsSuccess())
        {
            if (fabricUpgrade)
            {
                WriteInfo(TraceLifeCycle, "FabricUpgrade loaded: {0}", *fabricUpgrade);
            }

            error = fmStore_->GetKeyValues(keyValues);
        }

        if (error.IsSuccess())
        {
            WriteInfo(TraceLifeCycle, "KeyValues loaded: {0}", keyValues);

            ASSERT_IF(isReady_, "FM is ready during load.");

            if (!backgroundManager_)
            {
                nodeCache_ = make_unique<NodeCache>(*this, nodeInfos);

                for (auto it = applications.begin(); it != applications.end(); ++it)
                {
                    ApplicationUpgradeUPtr const & upgrade = (*it)->Upgrade;
                    if (upgrade)
                    {
                        upgrade->UpdateDomains(nodeCache_->GetUpgradeDomains(upgrade->UpgradeDomainSortPolicy));

                        WriteInfo(TraceApplicationUpgrade, wformatString(upgrade->Description.ApplicationId), "Loaded upgrade {0}", *upgrade);
                    }
                }

                plb_ = CreatePLB(nodeInfos, applications, serviceTypes, serviceInfos, failoverUnits, loadInfos);

                serviceCache_ = make_unique<ServiceCache>(*this, *fmStore_, *plb_, applications, serviceTypes, serviceInfos);
                loadCache_ = make_unique<LoadCache>(*this, *fmStore_, *plb_, loadInfos);

                for (auto it = failoverUnits.begin(); it != failoverUnits.end(); it++)
                {
                    (*it)->UpdatePointers(*this, *nodeCache_, *serviceCache_);
                }

                int64 savedLookupVersion;
                auto i = keyValues.find(Constants::LookupVersionKey);
                if (i != keyValues.end())
                {
                    savedLookupVersion = Int64_Parse(i->second);
                }
                else
                {
                    savedLookupVersion = 0;
                }

                i = keyValues.find(Constants::GenerationKey);
                if (i != keyValues.end())
                {
                    vector<wstring> generationStrings;
                    StringUtility::Split<wstring>(i->second, generationStrings, L":");

                    NodeId owner;
                    bool result = NodeId::TryParse(generationStrings[1], owner);

                    ASSERT_IFNOT(result, "FM {0} failed to parse NodeId: {1}", Id, i->second);

                    generation_ = GenerationNumber(Int64_Parse(generationStrings[0]), owner);
                }

                inBuildFailoverUnitCache_ = make_unique<InBuildFailoverUnitCache>(*this, move(inBuildFailoverUnits), *this);

                failoverUnitCache_ = make_unique<FailoverUnitCache>(*this, failoverUnits, savedLookupVersion, *this);

                if (fabricUpgrade)
                {
                    fabricUpgrade->UpdateDomains(nodeCache_->GetUpgradeDomains(fabricUpgrade->UpgradeDomainSortPolicy));
                    plb_->UpdateClusterUpgrade(true, fabricUpgrade->GetCompletedDomains());

                    WriteInfo(TraceFabricUpgrade, "Upgrade loaded: {0}", *fabricUpgrade);
                }
                else
                {
                    plb_->UpdateClusterUpgrade(false, set<wstring>());
                }

                fabricUpgradeManager_ = make_unique<FailoverManagerComponent::FabricUpgradeManager>(*this, move(currentVersionInstance), move(fabricUpgrade));

                messageProcessor_ = make_unique<FailoverUnitMessageProcessor>(*this);

                processingQueue_ = make_unique<FailoverUnitJobQueue>(*this);
                commitQueue_ = make_unique<CommitJobQueue>(*this);

                queryQueue_ = make_unique<CommonTimedJobQueue<FailoverManager>>(
                    L"FMQuery." + Id,
                    *this, // Root
                    false, // ForceEnqueue
                    FailoverConfig::GetConfig().QueryQueueThreadCount,
                    JobQueuePerfCounters::CreateInstance(L"FMQuery", Id),
                    FailoverConfig::GetConfig().QueryQueueSize);
                backgroundManager_ = make_unique<BackgroundManager>(*this, *this);
            }

            if (!fmQueryHelper_)
            {
                fmQueryHelper_ = make_unique<FMQueryHelper>(*this);
                error = fmQueryHelper_->Open();
                if (!error.IsSuccess())
                {
                    WriteWarning(TraceLifeCycle, "{0} query helper failed to open with error {1}", Id, error);
                }
            }

            if ((isMaster_ || (FailoverConfig::GetConfig().TargetReplicaSetSize > 0 && FailoverUnitCacheObj.Count == 0)) &&
                !FailoverConfig::GetConfig().IsTestMode)
            {
                if (!rebuildContext_)
                {
                    rebuildContext_ = make_unique<RebuildContext>(*this, lockObject_);
                    rebuildContext_->Start();
                }

                if (!isMaster_ && !networkInventoryServiceUPtr_)
                {
                    error = InitializeNetworkInventoryService();
                    if (!error.IsSuccess())
                    {
                        StartLoading(true);
                    }
                }

                return;
            }
            else
            {
                CompleteLoad();
            }
        }

        if (!error.IsSuccess())
        {
            WriteWarning(TraceLifeCycle, "{0} failed to load from store: {1}", Id, error);

            // Loading GFUM from store failed so we should retry just the loading.
            // Reopening the store is not necessary
            StartLoading(true);
        }
        else if (!networkInventoryServiceUPtr_)
        {
            error = InitializeNetworkInventoryService();
            if (!error.IsSuccess())
            {
                StartLoading(true);
            }
        }

    }
    else
    {
        WriteError(TraceLifeCycle, "{0} unable to load GFUM", Id);

        fmFailedEvent_->Fire(EventArgs(), true);
    }
}

Common::ErrorCode FailoverManager::InitializeNetworkInventoryService()
{
     Common::ErrorCode error;

    if (!Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
    {
        WriteInfo(TraceLifeCycle, "NetworkInventoryService setting is not enabled in this config. Skipping initialization");
        return error;
    }

    // Instantiate the NetworkInventoryService service.
    auto nis = make_unique<NetworkInventoryService>(*this);
    error = nis->Initialize();

    if (error.IsSuccess())
    {
        networkInventoryServiceUPtr_ = move(nis);
    }
    else
    {
        WriteInfo(TraceLifeCycle, "NetworkInventoryService failed to open with error {0}", error);
    }

    return error;
}

void FailoverManager::AdjustTimestamps(vector<LoadInfoSPtr> & loadInfos)
{
    StopwatchTime maxTime = StopwatchTime::Zero;

    for (auto itLoadInfo = loadInfos.begin(); itLoadInfo != loadInfos.end(); ++itLoadInfo)
    {
        LoadBalancingComponent::LoadOrMoveCostDescription const& loadDescription = (*itLoadInfo)->LoadDescription;
        if (loadDescription.IsStateful)
        {
            for (auto it = loadDescription.PrimaryEntries.begin(); it != loadDescription.PrimaryEntries.end(); ++it)
            {
                if (maxTime < it->LastReportTime)
                {
                    maxTime = it->LastReportTime;
                }
            }
        }
        for (auto it = loadDescription.SecondaryEntries.begin(); it != loadDescription.SecondaryEntries.end(); ++it)
        {
            if (maxTime < it->LastReportTime)
            {
                maxTime = it->LastReportTime;
            }
        }

        for (auto it = loadDescription.SecondaryEntriesMap.begin(); it != loadDescription.SecondaryEntriesMap.end(); ++it)
        {
            for (auto itVec = it->second.begin(); itVec != it->second.end(); ++itVec)
            {
                if (maxTime < itVec->LastReportTime)
                {
                    maxTime = itVec->LastReportTime;
                }
            }
        }

    }

    StopwatchTime now = Stopwatch::Now();

    if (now != maxTime)
    {
        TimeSpan diff = now - maxTime;

        for (auto itLoadInfo = loadInfos.begin(); itLoadInfo != loadInfos.end(); ++itLoadInfo)
        {
            (*itLoadInfo)->AdjustTimestamps(diff);
        }
    }
}

InstrumentedPLBUPtr FailoverManager::CreatePLB(
    vector<NodeInfoSPtr> const& nodes,
    std::vector<ApplicationInfoSPtr> const& applications,
    vector<ServiceTypeSPtr> const& serviceTypes,
    vector<ServiceInfoSPtr> const& services,
    vector<FailoverUnitUPtr> const& failoverUnits,
    vector<LoadInfoSPtr> const& loadInfos)
{
    vector<LoadBalancingComponent::NodeDescription> plbNodes;
    plbNodes.reserve(nodes.size());
    for (NodeInfoSPtr const& nodeInfo : nodes)
    {
        plbNodes.push_back(nodeInfo->GetPLBNodeDescription());
    }

    vector<LoadBalancingComponent::ApplicationDescription> plbApplications;
    plbApplications.reserve(applications.size());

    for (ApplicationInfoSPtr const & app : applications)
    {
        Common::NamingUri emptyName = Common::NamingUri();
        if (app->ApplicationName != emptyName)
        {
            plbApplications.push_back(app->GetPLBApplicationDescription());
        }
    }

    vector<LoadBalancingComponent::ServiceTypeDescription> plbServiceTypes;
    plbServiceTypes.reserve(serviceTypes.size());
    for (ServiceTypeSPtr const& serviceType : serviceTypes)
    {
        if (serviceType->Type.ServiceTypeName != Constants::TombstoneServiceType)
        {
            plbServiceTypes.push_back(serviceType->GetPLBServiceTypeDescription());
        }
    }

    vector<LoadBalancingComponent::ServiceDescription> plbServices;
    plbServices.reserve(services.size());
    for (ServiceInfoSPtr const& serviceInfo : services)
    {
        if (!serviceInfo->IsDeleted && serviceInfo->Name != Constants::TombstoneServiceName)
        {
            plbServices.push_back(serviceInfo->GetPLBServiceDescription());
        }
    }

    StopwatchTime now = Stopwatch::Now();
    vector<LoadBalancingComponent::FailoverUnitDescription> plbFailoverUnits;
    plbFailoverUnits.reserve(failoverUnits.size());
    for (FailoverUnitUPtr const& failoverUnit : failoverUnits)
    {
        if (!failoverUnit->IsOrphaned)
        {
            plbFailoverUnits.push_back(failoverUnit->GetPLBFailoverUnitDescription(now));
        }
    }

    vector<LoadBalancingComponent::LoadOrMoveCostDescription> plbLoads;
    for (LoadInfoSPtr const& load : loadInfos)
    {
        plbLoads.push_back(load->GetPLBLoadOrMoveCostDescription());
    }

    IPlacementAndLoadBalancingUPtr plb = InstrumentedPLB::Create(
        *this, // FM
        *this, // Root
        isMaster_,
        false, // MovementEnabled
        move(plbNodes),
        move(plbApplications),
        move(plbServiceTypes),
        move(plbServices),
        move(plbFailoverUnits),
        move(plbLoads),
        HealthClient,
        serviceManagementClient_,
        FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgradeEntry);

    return InstrumentedPLB::CreateInstrumentedPLB(*this, move(plb));
}

void FailoverManager::CompleteLoad()
{
    WriteInfo(TraceLifeCycle, "{0} load completed", Id);

    isReady_ = true;
    readyTime_ = DateTime::Now();
    fmReadyEvent_->Fire(EventArgs());
    
    Events.Ready(Id, federation_->Instance.Id, federation_->Instance.InstanceId, (Stopwatch::Now() - activateTime_));

    auto root = CreateComponentRoot();
    Threadpool::Post([this, root]
    {
        backgroundManager_->Start();
    });
}

void FailoverManager::CreateFMService()
{
    vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
    consistencyUnitDescriptions.push_back(ConsistencyUnitDescription(
        ConsistencyUnitId(Constants::FMServiceGuid)));

    ErrorCode error = ServiceCacheObj.CreateService(move(ServiceCache::GetFMServiceDescription()), move(consistencyUnitDescriptions), false);

    TESTASSERT_IF(!error.IsSuccess() && isActive_, "{0} failed to create service: {1}", Id, error);
}

SerializedGFUMUPtr FailoverManager::Close(bool isStoreCloseNeeded)
{
    AcquireExclusiveLock lock(lockObject_);

    ASSERT_IFNOT(isActive_, "FM {0} is not active when closed", Id);

    isReady_ = false;
    isActive_ = false;

    WriteInfo(TraceLifeCycle, "{0} is deactivating", Id);

    federation_->UnRegisterMessageHandler(messageProcessingActor_);

    if (fmQueryHelper_)
    {
        fmQueryHelper_->Close();
    }

    this->fmFailedEvent_->Close();
    this->fmReadyEvent_->Close();

    if (rebuildContext_)
    {
        rebuildContext_->Stop();
    }

    if (failoverUnitCache_ != nullptr)
    {
        failoverUnitCache_->ServiceLookupTable.Dispose();
    }

    if (plb_ != nullptr)
    {
        plb_->Dispose();
    }

    if (backgroundManager_ != nullptr)
    {
        backgroundManager_->Stop();
    }

    if (processingQueue_ != nullptr)
    {
        processingQueue_->Close();
    }

    if (commitQueue_ != nullptr)
    {
        commitQueue_->Close();
    }

    if (commonQueue_ != nullptr)
    {
        commonQueue_->Close();
    }

    if (queryQueue_ != nullptr)
    {
        queryQueue_->Close();
    }

    if (serviceCache_ != nullptr)
    {
        serviceCache_->Close();
    }

    if (inBuildFailoverUnitCache_ != nullptr)
    {
        inBuildFailoverUnitCache_->Dispose();
    }

    if (fmStore_ != nullptr)
    {
        WriteInfo(TraceLifeCycle, "{0} disposing store", Id);
        fmStore_->Dispose(isStoreCloseNeeded);
    }

    if (networkInventoryServiceUPtr_ != nullptr)
    {
        networkInventoryServiceUPtr_->Close();
    }

    // Save GFUM for potential transfer to the new FMM.
    unique_ptr<GFUMMessageBody> savedGFUM;
    if (isMaster_ && failoverUnitCache_)
    {
        LockedFailoverUnitPtr failoverUnit;
        if (FailoverUnitCacheObj.TryGetLockedFailoverUnit(FailoverUnitId(Constants::FMServiceGuid), failoverUnit) && failoverUnit)
        {
            std::vector<NodeInfo> nodes;
            ErrorCode error = nodeCache_->GetNodesForGFUMTransfer(nodes);

            if (error.IsSuccess())
            {
                ServiceInfoSPtr serviceInfo;
                error = ServiceCacheObj.GetFMServiceForGFUMTransfer(serviceInfo);

                if (error.IsSuccess())
                {
                    savedGFUM = make_unique<GFUMMessageBody>(
                        generation_,
                        move(nodes),
                        *failoverUnit,
                        serviceInfo->ServiceDescription,
                        failoverUnitCache_->ServiceLookupTable.EndVersion);
                }
            }
        }

        if (!savedGFUM)
        {
            WriteWarning("Rebuild", "Failed to save GFUM for transfer");
        }
    }

    Events.Deactivated(Id, federation_->Instance.Id, federation_->Instance.InstanceId);

    GFUMMessageBody * ptr = savedGFUM.get();
    savedGFUM.release();

    return SerializedGFUMUPtr(static_cast<Serialization::IFabricSerializable*>(ptr));
}

ErrorCode FailoverManager::SetGeneration(GenerationNumber const & value)
{
    ErrorCode error = Store.UpdateKeyValue(Constants::GenerationKey, value.ToString());

    if (error.IsSuccess())
    {
        WriteInfo("Rebuild", "{0} generation number updated to {1}", Id, value);

        generation_ = value;
    }
    else
    {
        WriteWarning("Rebuild", "{0} failed to persist GenerationNumber {1} with error {2}", Id, value, error);
    }

    return error;
}

HHandler FailoverManager::RegisterFailoverManagerReadyEvent(EventHandler const & handler)
{
    return fmReadyEvent_->Add(handler);
}

bool FailoverManager::UnRegisterFailoverManagerReadyEvent(HHandler hHandler)
{
    return fmReadyEvent_->Remove(hHandler);
}

HHandler FailoverManager::RegisterFailoverManagerFailedEvent(EventHandler const & handler)
{
    return fmFailedEvent_->Add(handler);
}

bool FailoverManager::UnRegisterFailoverManagerFailedEvent(HHandler hHandler)
{
    return fmFailedEvent_->Remove(hHandler);
}

class FailoverManager::QueryMessageHandlerJobItem : public CommonTimedJobItem<FailoverManager>
{
public:
    QueryMessageHandlerJobItem(
        Transport::MessageUPtr && message,
        TimedRequestReceiverContextUPtr && context,
        TimeSpan timeout)
        : CommonTimedJobItem(timeout)
        , message_(move(message))
        , context_(move(context))
    {
    }

    virtual void Process(FailoverManager & fm)
    {
        fm.GetQueryAsyncMessageHandler(message_, move(context_), RemainingTime);
    }

    virtual void OnTimeout(FailoverManager &fm)
    {
        fm.WriteInfo(
            TraceFailure,
            "[{0}] Request timed out. Action: {1}",
            FabricActivityHeader::FromMessage(*message_),
            message_->Action);
    }

    virtual void OnQueueFull(FailoverManager & fm, size_t actualQueueSize) override
    {
        // Prevent performance issues by only sending out the reject messages once every couple of seconds
        if (fm.QueueFullThrottle.IsThrottling())
        {
            fm.Events.OnQueueFull(QueueName::QueryQueue, actualQueueSize);

            context_->Reject(ErrorCodeValue::ServiceTooBusy);
        }
    }

private:
    Transport::MessageUPtr message_;
    TimedRequestReceiverContextUPtr context_;
};

bool FailoverManager::IsReadStatusGranted() const
{
    if (servicePartition_)
    {
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus;
        HRESULT hr = servicePartition_->GetReadStatus(&readStatus);
        if (FAILED(hr) || readStatus != FABRIC_SERVICE_PARTITION_ACCESS_STATUS::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
        {
            return false;
        }
    }

    return true;
}

void FailoverManager::OneWayMessageHandler(Transport::MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
{
    if (!IsReadStatusGranted())
    {
        oneWayReceiverContext->Reject(ErrorCodeValue::FMNotReadyForUse);
        return;
    }

    if (!IsReady && message->Action != RSMessage::GetGenerationUpdateReject().Action)
    {
        oneWayReceiverContext->Reject(ErrorCodeValue::FMNotReadyForUse);
        return;
    }

    bool processed = ProcessFailoverUnitRequest(*message, oneWayReceiverContext->FromInstance);
    if (processed)
    {
        oneWayReceiverContext->Accept();
    }
    else
    {
        TimeoutHeader timeoutHeader;
        TimeSpan timeout = TimeSpan::MaxValue;
        if (message->Headers.TryReadFirst(timeoutHeader))
        {
            timeout = timeoutHeader.Timeout;
        }
        else
        {
            WriteWarning(TraceReceive, "TimeoutHeader not found: {0}", message->Action);
        }

        commonQueue_->Enqueue(make_unique<OneWayMessageHandlerJobItem>(move(message), move(oneWayReceiverContext), timeout));
    }
}

void FailoverManager::RequestMessageHandler(Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
{
    if (!IsReadStatusGranted())
    {
        requestReceiverContext->Reject(ErrorCode(ErrorCodeValue::FMNotReadyForUse));
        return;
    }

    if (!IsReady &&
        message->Action != RSMessage::GetLFUMUpload().Action &&
        message->Action != RSMessage::GetChangeNotification().Action)
    {
        requestReceiverContext->Reject(ErrorCode(ErrorCodeValue::FMNotReadyForUse));
        return;
    }

    TimeoutHeader timeoutHeader;
    TimeSpan timeout = TimeSpan::MaxValue;
    if (message->Headers.TryReadFirst(timeoutHeader))
    {
        timeout = timeoutHeader.Timeout;
    }
    else
    {
        WriteWarning(TraceReceive, "TimeoutHeader not found: {0}", message->Action);
    }

    TimedRequestReceiverContextUPtr timedContext = make_unique<TimedRequestReceiverContext>(
        *this,
        move(requestReceiverContext),
        message->Action);

    if (message->Action == RSMessage::GetQueryRequest().Action)
    {
        queryQueue_->Enqueue(make_unique<QueryMessageHandlerJobItem>(move(message), move(timedContext), timeout));
    }
    else
    {
        commonQueue_->Enqueue(make_unique<RequestReplyMessageHandlerJobItem>(move(message), move(timedContext), timeout));
    }
}

bool FailoverManager::ProcessFailoverUnitRequest(Message & request, NodeInstance const & from)
{
    wstring const& action = request.Action;

    if (action == RSMessage::GetReplicaDropped().Action ||
        action == RSMessage::GetReplicaEndpointUpdated().Action)
    {
        FailoverUnitMessageTask<ReplicaMessageBody>::ProcessRequest(request, *this, from);
    }
    else if (action == RSMessage::GetAddInstanceReply().Action ||
        action == RSMessage::GetRemoveInstanceReply().Action ||
        action == RSMessage::GetAddReplicaReply().Action ||
        action == RSMessage::GetAddPrimaryReply().Action ||
        action == RSMessage::GetRemoveReplicaReply().Action ||
        action == RSMessage::GetDeleteReplicaReply().Action)
    {
        FailoverUnitMessageTask<ReplicaReplyMessageBody>::ProcessRequest(request, *this, from);
    }
    else if (action == RSMessage::GetDatalossReport().Action ||
        action == RSMessage::GetChangeConfiguration().Action)
    {
        FailoverUnitMessageTask<ConfigurationMessageBody>::ProcessRequest(request, *this, from);
    }
    else if (action == RSMessage::GetDoReconfigurationReply().Action)
    {
        FailoverUnitMessageTask<ConfigurationReplyMessageBody>::ProcessRequest(request, *this, from);
    }
    else
    {
        return false;
    }

    return true;
}

void FailoverManager::GetQueryAsyncMessageHandler(
    Transport::MessageUPtr & message,
    TimedRequestReceiverContextUPtr && context,
    TimeSpan const& timeout)
{
    auto activityId = FabricActivityHeader::FromMessage(*message).ActivityId;

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    MessageEvents.QueryRequest(activityId);

    auto operation = fmQueryHelper_->BeginProcessQueryMessage(
        *message,
        timeout,
        [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
        {
            OnProcessRequestComplete(activityId, operation, move(contextMover.TakeUPtr()));
        },
        CreateAsyncOperationRoot());
}

MessageUPtr FailoverManager::ProcessOneWayMessage(Message & message, NodeInstance const & from)
{
    Stopwatch stopwatch;
    stopwatch.Start();

    MessageEvents.MessageReceived(message.Action, id_, from.Id.ToString(), from.InstanceId);

    MessageUPtr reply = nullptr;

    wstring const& action = message.Action;

    if (action == RSMessage::GetReportLoad().Action)
    {
        reply = ReportLoadMessageHandler(message);
    }
    else if (action == RSMessage::GetNodeUpgradeReply().Action)
    {
        NodeUpgradeReplyHandler(message, from);
    }
    else if (action == RSMessage::GetCancelFabricUpgradeReply().Action)
    {
        CancelFabricUpgradeReplyHandler(message, from);
    }
    else if (action == RSMessage::GetCancelApplicationUpgradeReply().Action)
    {
        CancelApplicationUpgradeReplyHandler(message, from);
    }
    else if (action == RSMessage::GetNodeUpdateServiceReply().Action)
    {
        NodeUpdateServiceReplyHandler(message, from);
    }
    else if (action == RSMessage::GetGenerationUpdateReject().Action)
    {
        GenerationUpdateRejectMessageHandler(message, from);
    }
    else if (action == RSMessage::GetAvailableContainerImages().Action)
    {
        AvailableContainerImagesMessageHandler(message);
    }
    else
    {
        WriteWarning(TraceFailure, "Unknown action '{0}' for FM one way message.", action);
    }

    if (reply && action != RSMessage::GetLFUMUpload().Action)
    {
        reply->Headers.Add(GenerationHeader(Generation, isMaster_));
    }

    stopwatch.Stop();

    FailoverUnitCounters->MessageProcessDurationBase.Increment();
    FailoverUnitCounters->MessageProcessDuration.IncrementBy(static_cast<PerformanceCounterValue>(stopwatch.ElapsedMilliseconds));

    if (stopwatch.ElapsedMilliseconds > FailoverConfig::GetConfig().MessageProcessTimeLimit.TotalMilliseconds())
    {
        MessageEvents.SlowMessageProcess(action, stopwatch.ElapsedMilliseconds);
    }

    return reply;
}

MessageUPtr FailoverManager::ProcessRequestReplyMessage(Message & request, NodeInstance const & from)
{
    MessageEvents.MessageReceived(request.Action, id_, from.Id.ToString(), from.InstanceId);

    MessageUPtr reply = nullptr;

    wstring const& action = request.Action;

    if (action == RSMessage::GetNodeUp().Action)
    {
        reply = this->NodeUpMessageHandler(request, from);
    }
    else if (action == RSMessage::GetChangeNotification().Action)
    {
        reply = ChangeNotificationMessageHandler(request, from);
    }
    else if (action == RSMessage::GetServiceTableRequest().Action)
    {
        reply = ServiceTableRequestMessageHandler(request);
    }
    else if (action == RSMessage::GetDeactivateNode().Action)
    {
        reply = DeactivateNodeMessageHandler(request);
    }
    else if (action == RSMessage::GetActivateNode().Action)
    {
        reply = ActivateNodeMessageHandler(request);
    }
    else if (action == RSMessage::GetDeactivateNodesRequest().Action)
    {
        reply = DeactivateNodesMessageHandler(request);
    }
    else if (action == RSMessage::GetNodeDeactivationStatusRequest().Action)
    {
        reply = NodeDeactivationStatusRequestMessageHandler(request);
    }
    else if (action == RSMessage::GetRemoveNodeDeactivationRequest().Action)
    {
        reply = RemoveNodeDeactivationRequestMessageHandler(request);
    }
    else if (action == RSMessage::GetRecoverPartitions().Action)
    {
        reply = RecoverPartitionsMessageHandler(request);
    }
    else if (action == RSMessage::GetRecoverPartition().Action)
    {
        reply = RecoverPartitionMessageHandler(request);
    }
    else if (action == RSMessage::GetRecoverServicePartitions().Action)
    {
        reply = RecoverServicePartitionsMessageHandler(request);
    }
    else if (action == RSMessage::GetRecoverSystemPartitions().Action)
    {
        reply = RecoverSystemPartitionsMessageHandler(request);
    }
    else if (action == RSMessage::GetFabricUpgradeRequest().Action)
    {
        reply = FabricUpgradeRequestHandler(request);
    }
    else if (action == RSMessage::GetServiceDescriptionRequest().Action)
    {
        reply = ServiceDescriptionRequestMessageHandler(request);
    }
    else if (action == RSMessage::GetResetPartitionLoad().Action)
    {
        reply = ResetPartitionLoadMessageHandler(request);
    }
    else if (action == RSMessage::GetToggleVerboseServicePlacementHealthReporting().Action)
    {
        reply = ToggleVerboseServicePlacementHealthReportingMessageHandler(request);
    }
    else if (action == RSMessage::GetLFUMUpload().Action)
    {
        reply = LFUMUploadMessageHandler(request, from);
    }
    else
    {
        WriteWarning(TraceFailure, "Unknown action '{0}' for FM request reply message.", action);
    }

    if (reply && action != RSMessage::GetLFUMUpload().Action)
    {
        reply->Headers.Add(GenerationHeader(Generation, isMaster_));
    }

    return reply;
}

// Single-node version of DeactivateNode requests are no longer sent by the
// Naming gateway. These functions exist for backwards compatibilty.
//
MessageUPtr FailoverManager::DeactivateNodeMessageHandler(Message & request)
{
    DeactivateNodeRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.DeactivateNodeRequest(body.NodeId, activityId, body.SequenceNumber, wformatString(body.DeactivationIntent));

    map<NodeId, NodeDeactivationIntent::Enum> nodesToDeactivate;
    ErrorCode error = NodeDeactivationIntent::FromPublic(body.DeactivationIntent, nodesToDeactivate[body.NodeId]);

    if (error.IsSuccess())
    {
        error = NodeCacheObj.DeactivateNodes(nodesToDeactivate, body.NodeId.ToString(), body.SequenceNumber);
    }

    MessageEvents.DeactivateNodeReply(body.NodeId, activityId, error);

    MessageUPtr reply = RSMessage::GetDeactivateNodeReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::ActivateNodeMessageHandler(Message & request)
{
    ActivateNodeRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.ActivateNodeRequest(body.NodeId, activityId, body.SequenceNumber);

    ErrorCode error = NodeCacheObj.RemoveNodeDeactivation(body.NodeId.ToString(), body.SequenceNumber);

    MessageEvents.ActivateNodeReply(body.NodeId, activityId, error);

    MessageUPtr reply = RSMessage::GetActivateNodeReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::DeactivateNodesMessageHandler(Message & request)
{
    DeactivateNodesRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    WriteInfo(
        "DeactivateNode", body.BatchId,
        "Processing DeactivateNodes: {0}", body);

    ErrorCode error = NodeCacheObj.DeactivateNodes(body.Nodes, body.BatchId, body.SequenceNumber);

    WriteInfo(
        "DeactivateNode", body.BatchId,
        "DeactivateNodes for batch {0} completed with error {1}.", body.BatchId, error);

    MessageUPtr reply = RSMessage::GetDeactivateNodesReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::NodeDeactivationStatusRequestMessageHandler(Message & request)
{
    NodeDeactivationStatusRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    WriteInfo(
        "DeactivateNode", body.BatchId,
        "Processing NodeDeactivationStatus request for batch {0}", body.BatchId);

    NodeDeactivationStatus::Enum nodeDeactivationStatus;
    vector<NodeProgress> progressDetails;
    ErrorCode error = NodeCacheObj.GetNodeDeactivationStatus(body.BatchId, nodeDeactivationStatus, progressDetails);

    WriteInfo(
        "DeactivateNode", body.BatchId,
        "NodeDeactivationStatus request for batch {0} completed with status {1} and error {2}.\r\n{3}",
        body.BatchId, nodeDeactivationStatus, error, progressDetails);

    NodeDeactivationStatusReplyMessageBody replyBody(error.ReadValue(), nodeDeactivationStatus, move(progressDetails));
    MessageUPtr reply = RSMessage::GetNodeDeactivationStatusReply().CreateMessage(replyBody);
    return reply;
}

MessageUPtr FailoverManager::RemoveNodeDeactivationRequestMessageHandler(Message & request)
{
    RemoveNodeDeactivationRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    WriteInfo(
        "DeactivateNode", body.BatchId,
        "Processing RemoveNodeDeactivation for batch {0} with sequence number {1}", body.BatchId, body.SequenceNumber);

    ErrorCode error = NodeCacheObj.RemoveNodeDeactivation(body.BatchId, body.SequenceNumber);

    WriteInfo(
        "DeactivateNode", body.BatchId,
        "RemoveNodeDeactivation request for batch {0} completed with error {1}.", body.BatchId, error);

    MessageUPtr reply = RSMessage::GetRemoveNodeDeactivationReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

void FailoverManager::NodeDeactivateReplyAsyncMessageHandler(Message & request, Federation::NodeInstance const& from)
{
    NodeDeactivateReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    NodeCacheObj.ProcessNodeDeactivateReplyAsync(from.Id, body.SequenceNumber);
}

void FailoverManager::NodeActivateReplyAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from)
{
    NodeActivateReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    NodeCacheObj.ProcessNodeActivateReplyAsync(from.Id, body.SequenceNumber);
}

void FailoverManager::NodeStateRemovedAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    NodeStateRemovedRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);
    NodeId const& nodeId = body.NodeId;

    MessageEvents.NodeStateRemovedRequest(activityId, nodeId);

    bool isFound = false;
    Common::ErrorCode error = ErrorCodeValue::Success;
    bool isCompleted = false;

    NodeInfoSPtr nodeInfo = NodeCacheObj.GetNode(nodeId);
    if (nodeInfo)
    {
        isFound = true;

        if (nodeInfo->IsUp)
        {
            error = ErrorCodeValue::NodeIsUp;
            isCompleted = true;
        }
        else if (nodeInfo->IsNodeStateRemoved)
        {
            isCompleted = true;
        }
    }
    else
    {
        isCompleted = true;
    }

    auto visitor = FailoverUnitCacheObj.CreateVisitor();
    while (!isCompleted)
    {
        FailoverUnitId failoverUnitId;
        bool result = false;
        LockedFailoverUnitPtr failoverUnit = visitor->MoveNext(result, failoverUnitId);

        if (!result)
        {
            // If we cannot iterate over all the FailoverUnits, we cannot be sure if all the replicas
            // on the node whose state was lost have been dropped. Hence, we fail the operation.

            error = ErrorCode::FirstError(error, ErrorCodeValue::UpdatePending);
        }
        else if (failoverUnit)
        {
            for (auto replica = failoverUnit->BeginIterator; replica != failoverUnit->EndIterator; ++replica)
            {
                if (replica->NodeInfoObj->NodeInstance.Id == nodeId)
                {
                    isFound = true;

                    if (!replica->IsDropped || !replica->IsDeleted)
                    {
                        failoverUnit.EnableUpdate();
                        failoverUnit->OnReplicaDropped(*replica);
                        replica->IsDeleted = true;
                    }
                }
            }

            if (failoverUnit->PersistenceState != PersistenceState::NoChange)
            {
                ErrorCode e = FailoverUnitCacheObj.UpdateFailoverUnit(failoverUnit);
                if (e.IsSuccess())
                {
                    FTEvents.TraceFTUpdateNodeStateRemoved(failoverUnit);
                }
                else
                {
                    error = ErrorCode::FirstError(error, e);
                }
            }
        }
        else
        {
            isCompleted = true;
        }
    }

    if (!isFound)
    {
        isFound = InBuildFailoverUnitCacheObj.InBuildReplicaExistsForNode(nodeId);
    }

    if (error.IsSuccess())
    {
        if (isFound)
        {
            if (nodeInfo)
            {
                MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

                NodeCacheObj.BeginNodeStateRemoved(
                    nodeInfo->Id,
                    [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
                    {
                        ErrorCode error = NodeCacheObj.EndNodeStateRemoved(operation);

                        OnNodeStateRemovedCompleted(move(contextMover.TakeUPtr()), activityId, error);
                    },
                    CreateAsyncOperationRoot());

                return;
            }
        }
        else
        {
            error = ErrorCodeValue::NodeNotFound;
        }
    }

    OnNodeStateRemovedCompleted(move(context), activityId, error);
}

void FailoverManager::OnNodeStateRemovedCompleted(
    TimedRequestReceiverContextUPtr && context,
    ActivityId const& activityId,
    ErrorCode error)
{
    MessageEvents.NodeStateRemovedReply(activityId, error);

    MessageUPtr reply = RSMessage::GetNodeStateRemovedReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    context->Reply(move(reply));
}

MessageUPtr FailoverManager::RecoverPartitionsMessageHandler(Message & request)
{
    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.RecoverPartitionsRequest(activityId);

    ErrorCode error = InBuildFailoverUnitCacheObj.RecoverPartitions();

    ErrorCode e = RecoverPartitions(
        [](FailoverUnit const& failoverUnit) -> bool
        {
            UNREFERENCED_PARAMETER(failoverUnit);

            return true;
        });

    if (!e.IsSuccess())
    {
        error = ErrorCode::FirstError(error, e);
    }

    MessageEvents.RecoverPartitionsReply(activityId, error);

    MessageUPtr reply = RSMessage::GetRecoverPartitionsReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::RecoverPartitionMessageHandler(Message & request)
{
    RecoverPartitionRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.RecoverPartitionRequest(activityId, body.PartitionId);

    ErrorCode error = InBuildFailoverUnitCacheObj.RecoverPartition(body.PartitionId);

    if (error.IsError(ErrorCodeValue::PartitionNotFound))
    {
        LockedFailoverUnitPtr failoverUnit;
        bool result = FailoverUnitCacheObj.TryGetLockedFailoverUnit(body.PartitionId, failoverUnit);

        if (result)
        {
            if (failoverUnit)
            {
                error = RecoverPartition(failoverUnit);
            }
        }
        else
        {
            error = ErrorCodeValue::Timeout;
        }
    }

    MessageEvents.RecoverPartitionReply(activityId, error);

    MessageUPtr reply = RSMessage::GetRecoverPartitionReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::RecoverServicePartitionsMessageHandler(Message & request)
{
    RecoverServicePartitionsRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.RecoverServicePartitionsRequest(activityId, body.ServiceName);

    wstring internalServiceName = SystemServiceApplicationNameHelper::GetInternalServiceName(body.ServiceName);

    ErrorCode error = InBuildFailoverUnitCacheObj.RecoverServicePartitions(internalServiceName);

    bool isFound = !error.IsError(ErrorCodeValue::ServiceNotFound);
    if (!isFound)
    {
        error = ErrorCodeValue::Success;
    }

    ErrorCode e = ServiceCacheObj.IterateOverFailoverUnits(
        internalServiceName,
        [this](LockedFailoverUnitPtr & failoverUnit) -> ErrorCode
        {
            if (failoverUnit->IsQuorumLost())
            {
                return RecoverPartition(failoverUnit);
            }

            return ErrorCodeValue::Success;
        });

    if (!e.IsError(ErrorCodeValue::ServiceNotFound))
    {
        isFound = true;

        if (!e.IsSuccess())
        {
            error = ErrorCode::FirstError(error, e);
        }
    }

    if (!isFound)
    {
        error = ErrorCode::FirstError(ErrorCodeValue::ServiceNotFound, error);
    }

    MessageEvents.RecoverServicePartitionsReply(activityId, error);

    MessageUPtr reply = RSMessage::GetRecoverServicePartitionsReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::RecoverSystemPartitionsMessageHandler(Message & request)
{
    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.RecoverSystemPartitionsRequest(activityId);

    ErrorCode error = InBuildFailoverUnitCacheObj.RecoverSystemPartitions();

    ErrorCode e = RecoverPartitions(
        [](FailoverUnit const& failoverUnit) -> bool
        {
            return failoverUnit.ServiceInfoObj->ServiceType->Type.IsSystemServiceType();
        });

    if (!e.IsSuccess())
    {
        error = ErrorCode::FirstError(error, e);
    }

    MessageEvents.RecoverSystemPartitionsReply(activityId, error);

    MessageUPtr reply = RSMessage::GetRecoverSystemPartitionsReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

ErrorCode FailoverManager::RecoverPartitions(function<bool(FailoverUnit const& failoverUnit)> const& predicate)
{
    ErrorCode error = ErrorCode::Success();

    auto visitor = FailoverUnitCacheObj.CreateVisitor();

    bool isCompleted = false;
    while (!isCompleted)
    {
        FailoverUnitId failoverUnitId;
        bool result = false;
        LockedFailoverUnitPtr failoverUnit = visitor->MoveNext(result, failoverUnitId);

        if (!result)
        {
            // If we cannot iterate over all the FailoverUnits, we cannot be sure if all the
            // partitions have been recovered. Hence, we fail the operation.

            error = ErrorCode::FirstError(error, ErrorCodeValue::Timeout);
        }
        else if (failoverUnit)
        {
            if (predicate(*failoverUnit) && failoverUnit->IsQuorumLost())
            {
                ErrorCode e = RecoverPartition(failoverUnit);

                if (!e.IsSuccess())
                {
                    error = ErrorCode::FirstError(error, e);
                }
            }
        }
        else
        {
            isCompleted = true;
        }
    }

    return error;
}

ErrorCode FailoverManager::RecoverPartition(LockedFailoverUnitPtr & failoverUnit)
{
    failoverUnit.EnableUpdate();

    failoverUnit->DropOfflineReplicas();

    ErrorCode error = FailoverUnitCacheObj.UpdateFailoverUnit(failoverUnit);

    if (error.IsSuccess())
    {
        FTEvents.TraceFTUpdateRecover(failoverUnit);
    }

    return error;
}

MessageUPtr FailoverManager::ServiceDescriptionRequestMessageHandler(Message & request)
{
    ServiceDescriptionRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.ServiceDescriptionRequest(activityId, body.ServiceName);

    wstring serviceName = ServiceModel::SystemServiceApplicationNameHelper::GetInternalServiceName(body.ServiceName);

    ServiceDescription serviceDescription;
    ErrorCode error = ServiceCacheObj.GetServiceDescription(serviceName, serviceDescription);
    if (!ServiceModel::SystemServiceApplicationNameHelper::IsInternalServiceName(serviceName))
    {
        serviceDescription.AddDefaultMetrics();
    }

    vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
    if (body.IncludeCuids)
    {
        error = FailoverUnitCacheObj.GetConsistencyUnitDescriptions(body.ServiceName, consistencyUnitDescriptions);
    }

    MessageEvents.ServiceDescriptionReply(activityId, error);

    MessageUPtr reply = RSMessage::GetServiceDescriptionReply().CreateMessage(ServiceDescriptionReplyMessageBody(
        serviceDescription,
        error.ReadValue(),
        move(consistencyUnitDescriptions)));

    return reply;
}

MessageUPtr FailoverManager::FabricUpgradeRequestHandler(Message & request)
{
    UpgradeFabricRequestMessageBody requestBody;
    if (!TryGetMessageBody(request, requestBody))
    {
        return nullptr;
    }

    WriteInfo(
        TraceFabricUpgrade, "Processing upgrade request: {0}", requestBody);

    UpgradeFabricReplyMessageBody replyBody;
    ErrorCode error = FabricUpgradeManager.ProcessFabricUpgrade(move(requestBody), replyBody);
    if (!error.IsSuccess())
    {
        replyBody.ErrorCodeValue = error.ReadValue();
    }

    WriteInfo(TraceFabricUpgrade, "Upgrade request completed with {0}", replyBody.ErrorCodeValue);

    return RSMessage::GetFabricUpgradeReply().CreateMessage(replyBody);
}

void FailoverManager::ApplicationUpgradeAsyncMessageHandler(
    Message & request, 
    TimedRequestReceiverContextUPtr && context)
{
    UpgradeApplicationRequestMessageBody requestBody;
    if (!TryGetMessageBody(request, requestBody))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    WriteInfo(
        TraceApplicationUpgrade, wformatString(requestBody.Specification.ApplicationId),
        "Processing upgrade request: {0}", requestBody);

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginProcessUpgrade(
        move(requestBody),
        [this, contextMover](AsyncOperationSPtr const& operation) mutable
        {
            auto processUpgradeOperation = AsyncOperation::Get<ServiceCache::ProcessUpgradeAsyncOperation>(operation);
            OnApplicationUpgradeCompleted(operation, processUpgradeOperation->ApplicationId, contextMover.TakeUPtr());
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnApplicationUpgradeCompleted(
    AsyncOperationSPtr const& operation,
    ApplicationIdentifier const& applicationId,
    TimedRequestReceiverContextUPtr const& context)
{
    UpgradeApplicationReplyMessageBody replyBody;
    ErrorCode error = ServiceCacheObj.EndProcessUpgrade(operation, replyBody);

    replyBody.ErrorCodeValue = error.ReadValue();

    WriteInfo(
        TraceApplicationUpgrade, wformatString(applicationId),
        "Upgrade request for {0} completed with {1}", applicationId, replyBody.ErrorCodeValue);

    MessageUPtr reply = RSMessage::GetApplicationUpgradeReply().CreateMessage(replyBody);
    context->Reply(move(reply));
}

MessageUPtr FailoverManager::NodeUpMessageHandler(Message & request, NodeInstance const & from)
{
    NodeUpMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    WriteInfo(Constants::NodeUpSource, wformatString(from.Id),
        "Processing NodeUp from {0}: {1}",
        from, body);

    Common::FabricVersionInstance targetVersionInstance;
    vector<UpgradeDescription> upgrades;

    ErrorCode error = NodeUp(from, body, targetVersionInstance, upgrades);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FMInvalidRolloutVersion))
    {
        WriteWarning(Constants::NodeUpSource, wformatString(from.Id), "NodeUp from {0} failed with error {1}", from, error);
        return nullptr;
    }

    NodeDeactivationInfo nodeDeactivationInfo = NodeCacheObj.GetNodeDeactivationInfo(from.Id);

    bool isNodeActivated = (!nodeDeactivationInfo.IsDeactivated || nodeDeactivationInfo.IsSafetyCheckInProgress);

    NodeUpAckMessageBody replyBody(move(targetVersionInstance), move(upgrades), isNodeActivated, nodeDeactivationInfo.SequenceNumber);

    WriteInfo(Constants::NodeUpSource, wformatString(from.Id), "Sending NodeUpAck message to {0}: {1}", from, replyBody);

    return RSMessage::GetNodeUpAck().CreateMessage(replyBody);
}

ErrorCode FailoverManager::NodeUp(
    NodeInstance const & from,
    NodeUpMessageBody const & body,
    _Out_ FabricVersionInstance & targetVersionInstance,
    _Out_ vector<UpgradeDescription> & upgrades)
{
    NodeDescription nodeDescription(
        body.VersionInstance,
        body.NodeUpgradeDomainId,
        body.NodeFaultDomainIds,
        body.NodePropertyMap,
        body.NodeCapacityRatioMap,
        body.NodeCapacityMap,
        body.NodeName,
        body.NodeType,
        body.IpAddressOrFQDN,
        body.ClusterConnectionPort,
        body.HttpGatewayPort);

    NodeInfoSPtr nodeInfo = make_shared<NodeInfo>(from, move(nodeDescription), !body.AnyReplicaFound);

    wstring upgradeDomainId = nodeInfo->ActualUpgradeDomainId;
    ErrorCode error = NodeCacheObj.NodeUp(move(nodeInfo), true /* IsVersionGatekeepingNeeded */, targetVersionInstance);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (ServicePackageInfo const & package : body.Packages)
    {
        CheckUpgrade(from, package, upgradeDomainId, upgrades);
    }

    if (body.AnyReplicaFound)
    {
        InBuildFailoverUnitCacheObj.AnyReplicaFound = true;

        if (!IsMaster)
        {
            error = ServiceCacheObj.CreateTombstoneService();
        }
    }

    if (error.IsSuccess())
    {
        error = InBuildFailoverUnitCacheObj.OnRebuildComplete();
    }

    if (error.IsSuccess())
    {
        AcquireExclusiveLock grab(lockObject_);
        if (rebuildContext_)
        {
            rebuildContext_->CheckRecovery();
        }
    }

    return error;
}

void FailoverManager::CheckUpgrade(
    NodeInstance const& from,
    ServicePackageInfo const & package,
    wstring const & upgradeDomain,
    std::vector<UpgradeDescription> & upgrades)
{
    ServicePackageVersionInstance versionInstance = ServicePackageVersionInstance::Invalid;
    bool isFound = serviceCache_->GetTargetVersion(package.PackageId, upgradeDomain, versionInstance);
    if (!isFound || versionInstance.InstanceId == package.VersionInstance.InstanceId)
    {
        return;
    }

    auto it = find_if(upgrades.begin(), upgrades.end(),
        [&package, &versionInstance](UpgradeDescription const & upgrade) { return (upgrade.ApplicationId == package.PackageId.ApplicationId && upgrade.InstanceId == versionInstance.InstanceId); });
    if (it == upgrades.end())
    {
        ApplicationUpgradeSpecification specification(
            package.PackageId.ApplicationId,
            versionInstance.Version.ApplicationVersionValue,
            L"" /*applicationName*/,
            versionInstance.InstanceId,
            UpgradeType::Rolling,
            false,
            false,
            vector<ServicePackageUpgradeSpecification>(),
            vector<ServiceTypeRemovalSpecification>(),
            map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> ());

        upgrades.push_back(UpgradeDescription(move(specification)));
        it = upgrades.end();
        --it;
    }

    wstring const & packageName = package.PackageId.ServicePackageName;

    RolloutVersion packageVersion;
    bool found = it->Specification.GetUpgradeVersionForPackage(packageName, packageVersion);
    if (!found)
    {
        it->Specification.AddPackage(
            packageName,
            versionInstance.Version.RolloutVersionValue,
            vector<wstring>()); // code package names
    }
    else
    {
        if (packageVersion != versionInstance.Version.RolloutVersionValue)
        {
            TRACE_AND_TESTASSERT(
                WriteError,
                Constants::NodeUpSource, wformatString(from.Id),
                "Rollout version mismatch {0}/{1} found for {2}",
                packageVersion,
                versionInstance.Version.RolloutVersionValue,
                package.PackageId);

            return;
        }
    }
}

bool FailoverManager::NodeDown(NodeInstance const & nodeInstance)
{
    {
        AcquireExclusiveLock grab(lockObject_);

        if (rebuildContext_)
        {
            rebuildContext_->NodeDown(nodeInstance);
        }

        if (!IsReady)
        {
            return false;
        }
    }

    ErrorCode result = nodeCache_->NodeDown(nodeInstance);
    return result.IsSuccess();
}

MessageUPtr FailoverManager::ChangeNotificationMessageHandler(Message & request, NodeInstance const& from)
{
    ChangeNotificationMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    bool failed = false;
    for (NodeInstance shutdownNode : body.ShutdownNodes)
    {
        failed |= !NodeDown(shutdownNode);
    }

    if (IsReady)
    {
        auto nodeInfo = NodeCacheObj.GetNode(from.Id);
        if (nodeInfo && nodeInfo->NodeInstance.InstanceId < from.InstanceId)
        {
            failed |= !NodeDown(nodeInfo->NodeInstance);
        }
    }

    if (!IsReady || failed)
    {
        WriteWarning(
            TraceFailure,
            "{0} failed to process ChangeNotificationMessage {1}",
            Id, body);

        return nullptr;
    }

    // The remaining alive nodes that fall in the range are in doubt.
    vector<NodeInstance> nodesInDoubt;
    vector<NodeInfoSPtr> nodes;
    NodeCacheObj.GetNodes(nodes);
    for (NodeInfoSPtr const& node : nodes)
    {
        if (node->IsUp && node->Id != from.Id && body.Token.Contains(node->Id))
        {
            nodesInDoubt.push_back(node->NodeInstance);
        }
    }

    if (nodesInDoubt.size() > 0)
    {
        // Challenge the nodes in doubt.

        ChangeNotificationChallenge changeNotificationChallangeBody(std::move(nodesInDoubt));

        return RSMessage::GetChangeNotificationChallenge().CreateMessage(changeNotificationChallangeBody);
    }

    return RSMessage::GetChangeNotificationAck().CreateMessage();
}

void FailoverManager::CreateServiceAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    CreateServiceMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.CreateServiceRequest(activityId, body.ServiceDescription.Name, body.ServiceDescription.Instance);

    ErrorCode error(ErrorCodeValue::Success);

    if (body.ServiceDescription.Type.IsSystemServiceType() &&
        ServiceCacheObj.ServiceExists(Constants::TombstoneServiceName) &&
        body.ServiceDescription.Type.ApplicationId != *ApplicationIdentifier::FabricSystemAppId) // Work around RDBug#1449091
    {
        // TombstoneService is created whenever any replica is found during rebuild.
        // Hence, the existence of TombstoneService implies that system services must have existed.
        // If so, we do not allow re-creation of system services, and wait for these services to be recovered.
        error = ErrorCodeValue::FMServiceAlreadyExists;
    }
    else if (InBuildFailoverUnitCacheObj.InBuildFailoverUnitExistsForService(body.ServiceDescription.Name))
    {
        error = InBuildFailoverUnitCacheObj.MarkInBuildFailoverUnitsToBeDeletedForService(body.ServiceDescription.Name);
        if (error.IsSuccess())
        {
            error = ErrorCodeValue::FMServiceDeleteInProgress;
        }
    }

    if (error.IsSuccess())
    {
        MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

        ServiceCacheObj.BeginCreateService(
            move(const_cast<ServiceDescription &>(body.ServiceDescription)),
            move(const_cast<vector<ConsistencyUnitDescription> &>(body.ConsistencyUnitDescriptions)),
            false, // IsCreateFromRebuild
            false, // IsServiceUpdateNeeded
            [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
            {
                ErrorCode error = ServiceCacheObj.EndCreateService(operation);
                OnCreateServiceCompleted(move(contextMover.TakeUPtr()), activityId, error);
            },
            CreateAsyncOperationRoot());
    }
    else
    {
        OnCreateServiceCompleted(move(context), activityId, error);
    }
}

void FailoverManager::OnCreateServiceCompleted(TimedRequestReceiverContextUPtr && context, ActivityId const& activityId, ErrorCode error)
{
    MessageEvents.CreateServiceReply(activityId, error);

    CreateServiceReplyMessageBody replyBody = CreateServiceReplyMessageBody(error.ReadValue(), ServiceDescription());
    MessageUPtr reply = RSMessage::GetCreateServiceReply().CreateMessage(replyBody);
    context->Reply(move(reply));
}

void FailoverManager::UpdateServiceAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    UpdateServiceMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.UpdateServiceRequest(activityId, body.ServiceDescription.Name, body.ServiceDescription.Instance, body.ServiceDescription.UpdateVersion);

    if (!IsReady)
    {
        context->Reject(ErrorCodeValue::FMNotReadyForUse);
        return;
    }

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginUpdateService(
        move(const_cast<ServiceDescription &>(body.ServiceDescription)),
        move(const_cast<vector<ConsistencyUnitDescription> &>(body.AddedCuids)),
        move(const_cast<vector<ConsistencyUnitDescription> &>(body.RemovedCuids)),
        [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndUpdateService(operation);
            OnUpdateServiceCompleted(move(contextMover.TakeUPtr()), activityId, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnUpdateServiceCompleted(TimedRequestReceiverContextUPtr && context, ActivityId const& activityId, ErrorCode error)
{
    MessageEvents.UpdateServiceReply(activityId, error);

    MessageUPtr reply = RSMessage::GetUpdateServiceReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    context->Reply(move(reply));
}

void FailoverManager::UpdateSystemServiceAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    UpdateSystemServiceMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.UpdateSystemServiceRequest(activityId, body.ServiceName);

    if (!IsReady)
    {
        context->Reject(ErrorCodeValue::FMNotReadyForUse);
        return;
    }

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginUpdateSystemService(
        move(const_cast<wstring &>(body.ServiceName)),
        move(const_cast<Naming::ServiceUpdateDescription &>(body.ServiceUpdateDescription)),
        [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndUpdateService(operation);
            OnUpdateServiceCompleted(move(contextMover.TakeUPtr()), activityId, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::DeleteServiceAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    DeleteServiceMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);
    wstring const& serviceName = body.Value;

    MessageEvents.DeleteServiceRequest(activityId, serviceName, body.InstanceId, body.IsForce);

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginDeleteService(
        serviceName,
        body.IsForce,
        body.InstanceId,
        [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndDeleteService(operation);
            OnDeleteServiceCompleted(move(contextMover.TakeUPtr()), activityId, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnDeleteServiceCompleted(TimedRequestReceiverContextUPtr && context, ActivityId const& activityId, ErrorCode error)
{
    MessageEvents.DeleteServiceReply(activityId, error);

    BasicFailoverReplyMessageBody replyBody = BasicFailoverReplyMessageBody(error.ReadValue());
    MessageUPtr reply = RSMessage::GetDeleteServiceReply().CreateMessage(replyBody);
    context->Reply(move(reply));
}

void FailoverManager::CreateApplicationAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    Common::ErrorCode error(ErrorCodeValue::Success);
    CreateApplicationRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.CreateApplicationRequest(activityId, body.ApplicationId, body.InstanceId);

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginCreateApplication(
        body.ApplicationId,
        body.InstanceId,
        body.ApplicationName,
        0,  // updateId
        body.ApplicationCapacity,
        body.ResourceGovernanceDescription,
        body.CodePackageContainersImages,
        body.Networks,
        [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndCreateApplication(operation);
            OnCreateApplicationCompleted(move(contextMover.TakeUPtr()), activityId, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnCreateApplicationCompleted(TimedRequestReceiverContextUPtr && context, ActivityId const& activityId, ErrorCode error)
{
    MessageEvents.CreateApplicationReply(activityId, error);

    MessageUPtr reply = RSMessage::GetCreateApplicationReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    context->Reply(move(reply));
}

void FailoverManager::DeleteApplicationAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    DeleteApplicationRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.DeleteApplicationRequest(activityId, body.ApplicationId, body.InstanceId);

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginDeleteApplication(
        body.ApplicationId,
        body.InstanceId,
        [this, activityId, contextMover](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndDeleteApplication(operation);
            OnDeleteApplicationCompleted(move(contextMover.TakeUPtr()), activityId, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnDeleteApplicationCompleted(TimedRequestReceiverContextUPtr && context, ActivityId const& activityId, ErrorCode error)
{
    MessageEvents.DeleteApplicationReply(activityId, error);

    BasicFailoverReplyMessageBody replyBody = BasicFailoverReplyMessageBody(error.ReadValue());
    MessageUPtr reply = RSMessage::GetDeleteApplicationReply().CreateMessage(replyBody);
    context->Reply(move(reply));
}

MessageUPtr FailoverManager::ServiceTableRequestMessageHandler(Message& request)
{
    ServiceTableRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.ResolutionRequest(activityId, wformatString(body));

    auto pageSizeLimit = static_cast<size_t>(
        Federation::FederationConfig::GetConfig().SendQueueSizeLimit * FailoverConfig::GetConfig().MessageContentBufferRatio);

    vector<ServiceTableEntry> entriesToUpdate;

    VersionRangeCollection versionRangesToUpdate;
    int64 endVersion;
    bool result = FailoverUnitCacheObj.ServiceLookupTable.GetUpdates(
        pageSizeLimit,
        body.Generation,
        body.ConsistencyUnitIds,
        move(const_cast<VersionRangeCollection &>(body.VersionRangeCollection)),
        entriesToUpdate,
        versionRangesToUpdate,
        endVersion);
    if (!result)
    {
        return nullptr;
    }

    MessageEvents.ResolutionReply(activityId, wformatString(versionRangesToUpdate), static_cast<uint64>(entriesToUpdate.size()));

    ServiceTableUpdateMessageBody replyBody(move(entriesToUpdate), generation_, move(versionRangesToUpdate), endVersion, IsMaster);
    return RSMessage::GetServiceTableUpdate().CreateMessage(replyBody);
}

void FailoverManager::NodeFabricUpgradeReplyAsyncMessageHandler(Message & request, NodeInstance const& from)
{
    NodeFabricUpgradeReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    WriteInfo(FailoverManager::TraceFabricUpgrade,
        "Processing upgrade reply from {0}: {1}",
        from, body);

    FabricUpgradeManager.ProcessNodeFabricUpgradeReplyAsync(move(body), from);
}

void FailoverManager::NodeUpgradeReplyHandler(Message & request, NodeInstance const& from)
{
    NodeUpgradeReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    WriteInfo(
        FailoverManager::TraceApplicationUpgrade, wformatString(body.ApplicationId),
        "Processing NodeUpgradeReply from {0}: {1}", from, body);

    ErrorCode error = NodeCacheObj.RemovePendingApplicationUpgrade(from, body.ApplicationId);
    if (error.IsSuccess())
    {
        error = ServiceCacheObj.ProcessUpgradeReply(body.ApplicationId, body.InstanceId, from);
    }

    if (!error.IsSuccess())
    {
        WriteWarning(
            FailoverManager::TraceApplicationUpgrade, wformatString(body.ApplicationId),
            "NodeUpgradeReply processing from {0} failed with error {1}: {2}", from, error, body);
    }
}

void FailoverManager::CancelFabricUpgradeReplyHandler(Message & request, NodeInstance const& from)
{
    NodeFabricUpgradeReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    WriteInfo(TraceFabricUpgrade, "Processing CancelFabricUpgradeReply from {0}: {1}", from, body);

    FabricUpgradeManager.ProcessCancelFabricUpgradeReply(move(body), from);
}

void FailoverManager::CancelApplicationUpgradeReplyHandler(Message & request, NodeInstance const& from)
{
    NodeUpgradeReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    ErrorCode error = NodeCacheObj.RemovePendingApplicationUpgrade(from, body.ApplicationId);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceApplicationUpgrade, "CancelApplicationUpgradeReply processing failed with error {0}", error);
    }
}

void FailoverManager::ReplicaUpAsyncMessageHandler(Message & request, NodeInstance const& from)
{
    ReplicaUpMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    shared_ptr<ReplicaUpProcessingOperation> operation = make_shared<ReplicaUpProcessingOperation>(*this, from);
    operation->Process(operation, *this, body);
}

void FailoverManager::ReplicaDownAsyncMessageHandler(Message & request, NodeInstance const& from)
{
    ReplicaListMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    shared_ptr<ReplicaDownOperation> operation = make_shared<ReplicaDownOperation>(*this, from);
    operation->Start(operation, *this, move(body));
}

void FailoverManager::NodeUpdateServiceReplyHandler(Message & request, NodeInstance const& from)
{
    NodeUpdateServiceReplyMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    ServiceCacheObj.ProcessNodeUpdateServiceReply(body, from);
}

void FailoverManager::ServiceTypeEnabledAsyncMessageHandler(
    Message & request,
    NodeInstance const& from)
{
    UpdateServiceTypeAsyncMessageHandler(request, move(from), ServiceTypeUpdateKind::Enabled);
}

void FailoverManager::ServiceTypeDisabledAsyncMessageHandler(
    Message & request,
    NodeInstance const& from)
{
    UpdateServiceTypeAsyncMessageHandler(request, move(from), ServiceTypeUpdateKind::Disabled);
}

void FailoverManager::UpdateServiceTypeAsyncMessageHandler(
    Message & request,
    NodeInstance const& from,
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent)
{
    ServiceTypeUpdateMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    auto messageId = request.MessageId;
    uint64 sequenceNumber = body.SequenceNumber;

    if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Enabled)
    {
        MessageEvents.ServiceTypeEnabledRequest(messageId, from, body.SequenceNumber);
    }
    else if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Disabled)
    {
        MessageEvents.ServiceTypeDisabledRequest(messageId, from, body.SequenceNumber);
    }

    auto serviceTypeIds = make_unique<vector<ServiceTypeIdentifier>>(move(body.ServiceTypes));
    MoveUPtr<vector<ServiceTypeIdentifier>> serviceTypeIdsMover(move(serviceTypeIds));

    NodeCacheObj.BeginUpdateNodeSequenceNumber(
        from,
        body.SequenceNumber,
        [this, serviceTypeUpdateEvent, from, messageId, sequenceNumber, serviceTypeIdsMover](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = NodeCacheObj.EndUpdateNodeSequenceNumber(operation);
            if (error.IsSuccess())
            {
                auto commitJobItem = make_unique<NodeSequenceNumberCommitJobItem>(
                    serviceTypeUpdateEvent,
                    move(*serviceTypeIdsMover.TakeUPtr()),
                    sequenceNumber,
                    messageId,
                    from);
                CommonQueue.Enqueue(move(commitJobItem));
            }
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnNodeSequenceNumberUpdated(
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
    vector<ServiceTypeIdentifier> && serviceTypes,
    uint64 sequenceNum,
    MessageId const& messageId,
    NodeInstance const& from)
{
    ServiceCacheObj.BeginUpdateServiceTypes(
        move(const_cast<vector<ServiceTypeIdentifier> &>(serviceTypes)),
        sequenceNum,
        from.Id,
        serviceTypeUpdateEvent,
        [this, serviceTypeUpdateEvent, messageId, sequenceNum, from](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndUpdateServiceTypes(operation);

            OnUpdateServiceTypeCompleted(serviceTypeUpdateEvent, messageId, sequenceNum, from, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnUpdateServiceTypeCompleted(
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
    MessageId const& messageId,
    uint64 sequenceNum,
    NodeInstance const& from,
    ErrorCode error)
{
    if (error.IsSuccess())
    {
        MessageUPtr reply;

        if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Enabled)
        {
            reply = RSMessage::GetServiceTypeEnabledReply().CreateMessage(ServiceTypeUpdateReplyMessageBody(sequenceNum));
        }
        else if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Disabled)
        {
            reply = RSMessage::GetServiceTypeDisabledReply().CreateMessage(ServiceTypeUpdateReplyMessageBody(sequenceNum));
        }

        reply->Headers.Add(GenerationHeader(Generation, isMaster_));
        SendToNodeAsync(move(reply), from);
    }

    if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Enabled)
    {
        MessageEvents.ServiceTypeEnabledReply(messageId, error);
    }
    else if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Disabled)
    {
        MessageEvents.ServiceTypeDisabledReply(messageId, error);
    }
}

void FailoverManager::ServiceTypeNotificationAsyncMessageHandler(Message & request, NodeInstance const& from)
{
    ServiceTypeNotificationRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    auto messageId = request.MessageId;

    MessageEvents.ServiceTypeNotificationRequest(messageId, from, body);

    ServiceCacheObj.BeginProcessServiceTypeNotification(
        move(const_cast<vector<ServiceTypeInfo> &>(body.Infos)),
        [this, messageId, from](AsyncOperationSPtr const& operation) mutable
        {
            vector<ServiceTypeInfo> processedInfos;
            ServiceCacheObj.EndProcessServiceTypeNotification(operation, processedInfos);
            OnServiceTypeNotificationCompleted(messageId, move(processedInfos), from);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnServiceTypeNotificationCompleted(
    MessageId const& messageId,
    vector<ServiceTypeInfo> && processedInfos,
    NodeInstance const& from)
{
    ServiceTypeNotificationReplyMessageBody replyBody(move(processedInfos));

    MessageEvents.ServiceTypeNotificationReply(messageId, from, replyBody);

    MessageUPtr reply = RSMessage::GetServiceTypeNotificationReply().CreateMessage(replyBody);

    reply->Headers.Add(GenerationHeader(Generation, isMaster_));
    SendToNodeAsync(move(reply), from);
}

Transport::MessageUPtr FailoverManager::ReportLoadMessageHandler(Message & request)
{
    ReportLoadMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    TimeSpan diff = Stopwatch::Now() - body.SenderTime;
    for (auto it = body.Reports.begin(); it != body.Reports.end(); ++it)
    {
        //It is possible the Service is deleted after the instance/replica reports its load.
        //In this case we drop the message
        if (FailoverUnitCacheObj.IsFailoverUnitValid(FailoverUnitId(it->FailoverUnitId)))
        {
            it->AdjustTimestamps(diff);
            loadCache_->UpdateLoads(move(*it));
        }
        else
        {
            WriteWarning("ReportLoadMessageHandler",
                "FailoverUnit does not exists for {0}", FailoverUnitId(it->FailoverUnitId));
        }
    }

    //No ack/nack is needed for ReportLoad message
    return nullptr;
}

void FailoverManager::ProcessGFUMTransfer(GFUMMessageBody & body)
{
    WriteInfo(
        "Rebuild",
        "Processing GFUM transfer message: {0}", body);

    // Update the GenerationNumber of the FMM.
    ErrorCode error = SetGeneration(body.Generation);

    TESTASSERT_IF(!error.IsSuccess() && isActive_, "{0} failed to persist GenerationNumber during GFUM tranfser: {1}", Id, error);

    // Initialize NodeCache
    auto nodes = move(body.Nodes);
    nodeCache_->Initialize(move(nodes));

    // If an older version node is transferring the GFUM, it may not have the ServiceDescription
    ServiceDescription serviceDescription = body.ServiceDescription.Name.empty() ? ServiceCache::GetFMServiceDescription() : body.ServiceDescription;

    // Check if ServiceDescription update is needed
    bool isServiceUpdateNeeded = false;
    for (auto replica = body.FailoverUnit.BeginIterator; replica != body.FailoverUnit.EndIterator; replica++)
    {
        if (replica->ServiceUpdateVersion < serviceDescription.UpdateVersion)
        {
            isServiceUpdateNeeded = true;
            break;
        }
    }

    vector<ConsistencyUnitDescription> consistencyUnitDescriptions;
    consistencyUnitDescriptions.push_back(ConsistencyUnitDescription(body.FailoverUnit.Id.Guid));

    // Create the FM Service
    error = ServiceCacheObj.CreateServiceDuringRebuild(move(serviceDescription), move(consistencyUnitDescriptions), isServiceUpdateNeeded);

    TESTASSERT_IF(!error.IsSuccess() && isActive_, "{0} failed to create FM service during GFUM transfer: {1}", Id, error);

    // Update end lookup version
    FailoverUnitCacheObj.ServiceLookupTable.EndVersion = body.EndLookupVersion;

    // Create the FailoverUnit for FM Service
    ASSERT_IFNOT(body.FailoverUnit.Id.IsFM, "Incorrect FT during GFUM transfer: {0}", body);

    FailoverUnitUPtr failoverUnit = make_unique<FailoverUnit>(move(body.FailoverUnit));

    // Persist the FailoverUnit
    failoverUnit->PersistenceState = PersistenceState::ToBeInserted;
    int64 commitDuration;
    error = Store.UpdateData(*failoverUnit, commitDuration);
    if (error.IsSuccess())
    {
        failoverUnit->UpdatePointers(*this, *nodeCache_, *serviceCache_);
        FailoverUnitCacheObj.InsertFailoverUnitInCache(move(failoverUnit));
    }
    else
    {
        TESTASSERT_IF(isActive_, "{0} failed to update FT during GFUM transfer: {1}", Id, error);
    }

    LockedFailoverUnitPtr lockedFailoverUnit;
    FailoverUnitCacheObj.TryGetLockedFailoverUnit(FailoverUnitId(Constants::FMServiceGuid), lockedFailoverUnit);

    FTEvents.FTUpdateGFUMTransfer(lockedFailoverUnit->IdString, *lockedFailoverUnit);
}

void FailoverManager::GenerationUpdateRejectMessageHandler(Message & request, NodeInstance const & from)
{
    AcquireExclusiveLock grab(lockObject_);

    if (rebuildContext_)
    {
        rebuildContext_->ProcessGenerationUpdateReject(request, from);
    }
}

MessageUPtr FailoverManager::LFUMUploadMessageHandler(Message & request, NodeInstance const & from)
{
    LFUMMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    AcquireExclusiveLock grab(lockObject_);

    MessageUPtr reply = nullptr;

    if (rebuildContext_)
    {
        reply = rebuildContext_->ProcessLFUMUpload(body, from);
    }
    else if (IsReady)
    {
        reply = RSMessage::GetLFUMUploadReply().CreateMessage();
    }

    if (reply)
    {
        reply->Headers.Add(GenerationHeader(body.Generation, isMaster_));
    }

    return reply;
}

void FailoverManager::ProcessFailoverUnitMovements(map<Guid, FailoverUnitMovement> && failoverUnitMovements, LoadBalancingComponent::DecisionToken && decisionToken)
{
    Events.Movements(decisionToken.DecisionId, failoverUnitMovements.size());

    if (NodeCacheObj.IsClusterPaused)
    {
        plb_->OnDroppedPLBMovements(move(failoverUnitMovements), PlbMovementIgnoredReasons::ClusterPaused, decisionToken.DecisionId);
        return;
    }

    if (FailoverConfig::GetConfig().DropAllPLBMovements)
    {
        plb_->OnDroppedPLBMovements(move(failoverUnitMovements), PlbMovementIgnoredReasons::DropAllPLBMovementsConfigTrue, decisionToken.DecisionId);
        return;
    }

    for (auto itMovement = failoverUnitMovements.begin(); itMovement != failoverUnitMovements.end(); ++itMovement)
    {
        FailoverUnitId failoverUnitId(itMovement->second.FailoverUnitId);

        DynamicStateMachineTaskUPtr task(new MovementTask(*this, this->NodeCacheObj, move(itMovement->second), move(decisionToken)));

        bool result = failoverUnitCache_->TryProcessTaskAsync(failoverUnitId, task, this->Federation.Instance, true);
        if (!result)
        {
            plb_->OnDroppedPLBMovement(failoverUnitId.Guid, PlbMovementIgnoredReasons::FailoverUnitNotFound, decisionToken.DecisionId);
        }
    }
}

void FailoverManager::UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const& appId)
{
    this->Events.PLBSafetyCheckForApplicationReceived(appId);
    ServiceCacheObj.BeginProcessPLBSafetyCheck(
        ServiceModel::ApplicationIdentifier(appId),
        [this](AsyncOperationSPtr const& operation)
        {
            auto processPLBSafetyCheck = AsyncOperation::Get<ServiceCache::ProcessPLBSafetyCheckAsyncOperation>(operation);
            this->OnPLBSafetyCheckProcessingDone(operation, processPLBSafetyCheck->ApplicationId);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnPLBSafetyCheckProcessingDone(Common::AsyncOperationSPtr const& operation, ServiceModel::ApplicationIdentifier const& appId)
{
    ErrorCode error = ServiceCacheObj.EndProcessPLBSafetyCheck(operation);
    this->Events.PLBSafetyCheckForApplicationProcessed(appId, error);
    if (error.IsSuccess())
    {
        plb_->OnSafetyCheckAcknowledged(appId);
    }
}

void FailoverManager::UpdateFailoverUnitTargetReplicaCount(Common::Guid const & partitionId, int targetCount)
{
    FailoverUnitId failoverUnitId(partitionId);

    DynamicStateMachineTaskUPtr task(new AutoScalingTask(targetCount));

    failoverUnitCache_->TryProcessTaskAsync(failoverUnitId, task, this->Federation.Instance, true);
}

void FailoverManager::RegisterEvents(EventHandler const & readyEvent, EventHandler const & failureEvent)
{
    fmReadyEvent_->Add(readyEvent);
    fmFailedEvent_->Add(failureEvent);

    // Register the one way and request-reply message handlers
    auto root = CreateComponentRoot();
    federation_->RegisterMessageHandler(
        messageProcessingActor_,
        [this, root](Transport::MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
        {
            this->OneWayMessageHandler(message, oneWayReceiverContext);
        },
        [this, root](Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        {
            this->RequestMessageHandler(message, requestReceiverContext);
        },
        true /*dispatchOnTransportThread*/);
    WriteInfo(TraceLifeCycle, "Registered message handler for {0}", this->messageProcessingActor_);
}

void FailoverManager::SendToNodeAsync(MessageUPtr && message, NodeInstance target)
{
    PartnerNodeSPtr node = federation_->GetNode(target);
    if (node)
    {
        if (node->Instance.InstanceId == target.InstanceId)
        {
            message->Idempotent = false;
            federation_->PToPSend(move(message), node);
        }
        else
        {
            auto thisRoot = CreateComponentRoot();
            Threadpool::Post([this, target, thisRoot]() { this->NodeDown(target); });
        }

        return;
    }

    auto action = message->Action;

    FailoverConfig const & config = FailoverConfig::GetConfig();

    federation_->BeginRoute(
        std::move(message),
        target.Id,
        target.InstanceId,
        true,
        TimeSpan::MaxValue,
        config.SendToNodeTimeout,
        [this, action, target](AsyncOperationSPtr const& routeOperation)
        {
            this->RouteCallback(routeOperation, action, target);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::SendToNodeAsync(MessageUPtr && message, NodeInstance target, TimeSpan routeTimeout, TimeSpan timeout)
{
    auto action = message->Action;

    federation_->BeginRoute(
        std::move(message),
        target.Id,
        target.InstanceId,
        true,
        routeTimeout,
        timeout,
        [this, action, target](AsyncOperationSPtr const& routeOperation)
        {
            this->RouteCallback(routeOperation, action, target);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::RouteCallback(AsyncOperationSPtr const& routeOperation, wstring const & action, NodeInstance const & target)
{
    ErrorCode error = federation_->EndRoute(routeOperation);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceFailure,
            "{0} failed to send message {1} to node {2} with error {3}",
            Id, action, target, error);

        if (error.ReadValue() == ErrorCodeValue::RoutingNodeDoesNotMatchFault)
        {
            auto thisRoot = CreateComponentRoot();
            Threadpool::Post([this, target, thisRoot]() { this->NodeDown(target); });
        }
    }
}

void FailoverManager::Broadcast(MessageUPtr&& message)
{
    federation_->Broadcast(move(message));
}

void FailoverManager::TraceInvalidMessage(Message & message)
{
    ASSERT_IF(message.IsValid, "Message cannot be valid and be asked to trace as invalid");

    MessageEvents.InvalidMessageDropped(id_, message.MessageId, message.Action, message.GetStatus());
}

void FailoverManager::TraceQueueCounts() const
{
    bool isUpgradingFabric = FabricUpgradeManager.Upgrade != NULL;

    QueueCounts counts(
        isUpgradingFabric,
        CommonQueue.GetQueueLength(),
        QueryQueue.GetQueueLength(),
        ProcessingQueue.GetQueueLength(),
        CommitQueue.GetQueueLength(),
        CommonQueue.GetCompleted(),
        QueryQueue.GetCompleted(),
        ProcessingQueue.GetCompleted(),
        CommitQueue.GetCompleted());

    Events.QueueCounts(counts);

    FailoverUnitCounters->CommonQueueLength.Value = static_cast<Common::PerformanceCounterValue>(counts.CommonQueueLength);
    FailoverUnitCounters->QueryQueueLength.Value = static_cast<Common::PerformanceCounterValue>(counts.QueryQueueLength);
    FailoverUnitCounters->FailoverUnitQueueLength.Value = static_cast<Common::PerformanceCounterValue>(counts.FailoverUnitQueueLength);
    FailoverUnitCounters->CommitQueueLength.Value = static_cast<Common::PerformanceCounterValue>(counts.CommitQueueLength);
}

void FailoverManager::OnProcessRequestComplete(
    Common::ActivityId const & activityId,
    AsyncOperationSPtr const & asyncOperation,
    TimedRequestReceiverContextUPtr && requestContext)
{
    MessageUPtr reply;
    auto error = fmQueryHelper_->EndProcessQueryMessage(asyncOperation, reply);
    if (error.IsSuccess())
    {
        reply->Headers.Replace(FabricActivityHeader(activityId));
        requestContext->Reply(std::move(reply));
    }
    else
    {
        requestContext->Reject(error);
    }

    MessageEvents.QueryReply(activityId, error);
}

MessageUPtr FailoverManager::ResetPartitionLoadMessageHandler(Message & request)
{
    ResetPartitionLoadRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    Common::ErrorCode error = ErrorCodeValue::Success;


    LockedFailoverUnitPtr failoverUnit;
    bool result = FailoverUnitCacheObj.TryGetLockedFailoverUnit(body.PartitionId, failoverUnit);

    if (result)
    {
        if (failoverUnit)
        {
            if (failoverUnit->ServiceInfoObj->ServiceType->Type.IsSystemServiceType())
            {
                error = ErrorCodeValue::AccessDenied;
            }
            else
            {
                error = plb_->ResetPartitionLoad(failoverUnit->Id, failoverUnit->ServiceName, failoverUnit->IsStateful);

                if (error.IsSuccess())
                {
                    loadCache_->DeleteLoads(failoverUnit->Id);
                }
            }
        }
        else
        {
            error = ErrorCodeValue::PartitionNotFound;
        }
    }
    else
    {
        error = ErrorCodeValue::Timeout;
    }

    MessageUPtr reply = RSMessage::GetResetPartitionLoad().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

MessageUPtr FailoverManager::ToggleVerboseServicePlacementHealthReportingMessageHandler(Message & request)
{
    ToggleVerboseServicePlacementHealthReportingRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return nullptr;
    }

    ErrorCode error = plb_ ? plb_->ToggleVerboseServicePlacementHealthReporting(body.Enabled) : Common::ErrorCodeValue::PLBNotReady;

    MessageUPtr reply = RSMessage::GetToggleVerboseServicePlacementHealthReporting().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    return reply;
}

void FailoverManager::UpdateApplicationAsyncMessageHandler(Message & request, TimedRequestReceiverContextUPtr && context)
{
    ErrorCode error(ErrorCodeValue::Success);
    UpdateApplicationRequestMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        context->Reject(ErrorCodeValue::OperationFailed);
        return;
    }

    ActivityId const& activityId = RSMessage::GetActivityId(request);

    MessageEvents.UpdateApplicationRequest(activityId, body.ApplicationId, body.InstanceId, body.UpdateId);

    MoveUPtr<TimedRequestReceiverContext> contextMover(move(context));

    ServiceCacheObj.BeginUpdateApplication(
        body.ApplicationId,
        body.UpdateId,
        body.ApplicationCapacity,
        [this, activityId, contextMover, body](AsyncOperationSPtr const & operation) mutable
        {
            ErrorCode error = ServiceCacheObj.EndUpdateApplication(operation);
            OnUpdateApplicationCompleted(move(contextMover.TakeUPtr()), activityId, error);
        },
        CreateAsyncOperationRoot());
}

void FailoverManager::OnUpdateApplicationCompleted(TimedRequestReceiverContextUPtr && context, ActivityId const& activityId, ErrorCode error)
{
    MessageEvents.UpdateApplicationReply(activityId, error);

    MessageUPtr reply = RSMessage::GetUpdateApplicationReply().CreateMessage(BasicFailoverReplyMessageBody(error.ReadValue()));
    context->Reply(move(reply));
}

void FailoverManager::AvailableContainerImagesMessageHandler(Transport::Message & request)
{
    AvailableContainerImagesMessageBody body;
    if (!TryGetMessageBody(request, body))
    {
        return;
    }

    // Sending available images to PLB 
    this->plb_->UpdateAvailableImagesPerNode(body.NodeId, body.AvailableImages);
}