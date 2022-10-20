// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Client;
using namespace Reliability;
using namespace Federation;
using namespace std;
using namespace Hosting2;
using namespace ReconfigurationAgentComponent;
using namespace Store;

namespace
{
    StringLiteral const TraceLifeCycle("LifeCycle");
    StringLiteral const TraceMessageName("Message");
    StringLiteral const TraceHosting("Hosting");
}

IReliabilitySubsystemUPtr Reliability::ReliabilitySubsystemFactory(ReliabilitySubsystemConstructorParameters & parameters)
{
    return make_unique<ReliabilitySubsystem>(parameters);
}

ReliabilitySubsystem::ReliabilitySubsystem(ReliabilitySubsystemConstructorParameters & parameters)
    : IReliabilitySubsystem(parameters.Root, parameters.NodeConfig, parameters.WorkingDir),
      federation_(*parameters.Federation),
      traceId_(federation_.IdString),
      ipcServer_(*parameters.IpcServer),
      hostingSubsystem_(*parameters.HostingSubsystem),
      runtimeSharingHelper_(*parameters.RuntimeSharingHelper),
      federationWrapper_(*parameters.Federation, &serviceResolver_),
      serviceAdminClient_(federationWrapper_),
      serviceResolver_(federationWrapper_, *parameters.Root),
      lockObject_(),
      isOpened_(false),
      fm_(nullptr),
      savedGFUM_(nullptr),
      routingTokenChangedHHandler_(),
      nodeUpgradeDomainId_(parameters.NodeConfig->UpgradeDomainId),
      nodeFaultDomainIds_(parameters.NodeConfig->NodeFaultDomainIds),
      nodePropertyMap_(parameters.NodeConfig->NodeProperties),
      securitySettings_(*parameters.SecuritySettings),
      serviceFactorySPtr_(),
      fmUpdateHHandler_(0),
      ra_(nullptr),
      healthClient_(),
      clientFactory_()
{
    REGISTER_MESSAGE_HEADER(GenerationHeader);

    PopulateMetricsAdjustRG(nodeCapacityRatioMap_, parameters.NodeCapacityMap);
    PopulateMetricsAdjustRG(nodeCapacityMap_, parameters.NodeConfig->NodeCapacities);

    auto reliabilityWrapper = make_shared<ReliabilitySubsystemWrapper>(this);

    ReconfigurationAgentConstructorParameters raParameters = {0};
    raParameters.Root = &Root;
    raParameters.ReliabilityWrapper = reliabilityWrapper;
    raParameters.FederationWrapper = &federationWrapper_;
    raParameters.IpcServer = &ipcServer_;
    raParameters.Hosting = &hostingSubsystem_;
    raParameters.StoreFactory = parameters.StoreFactory;
    raParameters.Clock = nullptr;
    raParameters.ProcessTerminationService = parameters.ProcessTerminationService;
    raParameters.KtlLoggerNode = parameters.KtlLoggerNode;

    ra_ = ReconfigurationAgentFactory(raParameters);

    WriteInfo(TraceLifeCycle, "{0}: constructor called", Id);
}

ReliabilitySubsystem::~ReliabilitySubsystem()
{
    WriteInfo(TraceLifeCycle, "{0}: destructor called", Id);
}

ErrorCode ReliabilitySubsystem::OnOpen()
{
    WriteInfo(TraceLifeCycle, "ReliabilitySubsystem opening on node {0}", Instance);

    auto root = Root.CreateComponentRoot();

    RegisterFederationSubsystemEvents();

    if (healthClient_)
    {
        healthClient_->Open();
        federation_.SetHealthClient(healthClient_);
    }

    // Start RA
    NodeUpOperationFactory factory;
    auto error = ra_->Open(factory);
    if(!error.IsSuccess())
    {
        WriteWarning(TraceLifeCycle, "{0} failed to open the RA {1}", Instance, error);
        return error;
    }

    nodeUpOperationFactory_ = make_unique<const NodeUpOperationFactory>(move(factory));

    // register ra message handlers only after ra is open
    RegisterRAMessageHandlers();
    RegisterHostingSubsystemEvents();

    //Only if the FM is reliable is when we need to Create the Runtime and the Factory
    Common::ComPointer<IFabricRuntime> fabricRuntimeCPtr = runtimeSharingHelper_.GetRuntime();
    if (!fabricRuntimeCPtr)
    {
        WriteWarning(TraceLifeCycle, "{0} failed to get COM fabric runtime object, there must be a racing close/abort", Instance);

        return ErrorCodeValue::ObjectClosed;
    }

    serviceFactorySPtr_ = make_shared<FailoverManagerComponent::FailoverManagerServiceFactory>(
        *this,
        [this, root] (EventArgs const &)
        {
            OnFailoverManagerReady();
        },
        [this, root] (EventArgs const &)
        {
            OnFailoverManagerFailed();
        },
        Root);

    serviceFactoryCPtr_.SetNoAddRef(new Api::ComStatefulServiceFactory(Api::IStatefulServiceFactoryPtr(
        serviceFactorySPtr_.get(),
        serviceFactorySPtr_->CreateComponentRoot())));

    HRESULT hr = fabricRuntimeCPtr->RegisterStatefulServiceFactory(
        ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId->ServiceTypeName.c_str(),
        serviceFactoryCPtr_.GetRawPointer());

    if (!SUCCEEDED(hr))
    {
        WriteWarning(
            TraceLifeCycle, 
            "{0} failed to RegisterStatefulServiceFactory: hr = {1}", 
            Instance,
            hr); 

        return ErrorCode::FromHResult(hr);
    }

    // Register for FM change event
    this->fmUpdateHHandler_ = this->serviceResolver_.RegisterFMChangeEvent(
        [this, root] (ServiceTableUpdateMessageBody const & serviceTableUpdateMessageBody)
        {
            this->federationWrapper_.UpdateFMService(serviceTableUpdateMessageBody.ServiceTableEntries.front(), serviceTableUpdateMessageBody.Generation);
        });

    ServiceTableEntry fmEntry;
    GenerationNumber generation;

    if (this->serviceResolver_.TryGetCachedFMEntry(fmEntry, generation))
    {
        this->federationWrapper_.UpdateFMService(fmEntry, generation);
    }

    {
        AcquireExclusiveLock acquire(lockObject_);
        isOpened_ = true;
    }

    // Force a routing token check since event issued previously are ignored
    OnRoutingTokenChanged();

    // if auto detection is enabled refresh node capacities with real values for RG metrics
    if(LoadBalancingComponent::PLBConfig::GetConfig().AutoDetectAvailableResources)
    {
        RefreshNodeCapacity();
    }

    // Reliability subsystem has opened successfully, send the NodeUp message
    SendNodeUpToFMM();
    SendNodeUpToFM();
    
    WriteInfo(TraceLifeCycle, "ReliabilitySubsystem opened successfully on node {0}", Instance);

    return error;
}

ErrorCode ReliabilitySubsystem::OnClose()
{
    Cleanup();

    CleanupServiceFactory();

    return ErrorCodeValue::Success;
}

void ReliabilitySubsystem::OnAbort()
{
    this->OnClose().ReadValue();
}

void ReliabilitySubsystem::Cleanup()
{
    UnRegisterRAMessageHandlers();

    // Deactivate FM
    {
        AcquireExclusiveLock acquire(lockObject_);

        if (fm_)
        {
            savedGFUM_ = fm_->Close(true /* isStoreCloseNeeded */);
            fm_ = nullptr;
        }

        isOpened_ = false;
    }

    fmFailedEvent_.Close();
    fmReadyEvent_.Close();

    // Close RA
    ra_->Close();

    UnRegisterFederationSubsystemEvents();

    this->serviceResolver_.UnRegisterFMChangeEvent(this->fmUpdateHHandler_);

    serviceResolver_.Dispose();

    if (healthClient_)
    {
        healthClient_->Close();
    }
}

void ReliabilitySubsystem::CleanupServiceFactory()
{
    if (serviceFactorySPtr_)
    {
        serviceFactorySPtr_->Cleanup();

        // serviceFactoryCPtr_ is only used in the Open and Close and hence access to it
        // is protected via statemachine, otherwise we need to use lock to reset CPtr as 
        // CPtr is not thread-safe
        serviceFactoryCPtr_.SetNoAddRef(nullptr);
    }
}

void ReliabilitySubsystem::InitializeHealthReportingComponent(HealthReportingComponentSPtr const & healthClient)
{
    healthClient_ = healthClient;
}

void ReliabilitySubsystem::InitializeClientFactory(Api::IClientFactoryPtr const & clientFactory)
{
    clientFactory_ = clientFactory;
}

void ReliabilitySubsystem::RegisterRAMessageHandlers()
{
    auto root = Root.CreateComponentRoot();
    Federation.RegisterMessageHandler(
        RSMessage::RaActor,
        [this, root] (Transport::MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
        { 
            this->ra_->ProcessTransportRequest(message, move(oneWayReceiverContext));            
        },
        [this, root] (Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        { 
            this->RaRequestMessageHandler(message, requestReceiverContext); 
        },
        true/*dispatchOnTransportThread*/);
}

void ReliabilitySubsystem::RegisterFederationSubsystemEvents()
{
    // Register the one way and request-reply message handlers
    auto root = Root.CreateComponentRoot();

    // Register handler for service table update broadcast messages.
    // The reply can take some time, as it needs to take service resolver cache lock to update information,
    // so it is best to not dispatch it on the transport thread.
    Federation.RegisterMessageHandler(
        RSMessage::ServiceResolverActor,
        [this, root] (Transport::MessageUPtr & message, OneWayReceiverContextUPtr &)
        { 
            serviceResolver_.ProcessServiceTableUpdateBroadcast(*message, root);
        },
        [this, root] (Transport::MessageUPtr & message, RequestReceiverContextUPtr &)
        {
            WriteError(TraceMessageName, traceId_, "{0} dropping unexpected request message for service resolver: {1}", Instance, *message); 
        },
        false/*dispatchOnTransportThread*/);

    // Register the event handlers
    routingTokenChangedHHandler_ = Federation.RegisterRoutingTokenChangedEvent(
        [this, root] (EventArgs const &) 
        {
            OnRoutingTokenChanged();
        });
}

void ReliabilitySubsystem::UnRegisterRAMessageHandlers()
{
    Federation.UnRegisterMessageHandler(RSMessage::RaActor);
}

void ReliabilitySubsystem::UnRegisterFederationSubsystemEvents()
{
    Federation.UnRegisterMessageHandler(RSMessage::ServiceResolverActor);

    Federation.UnRegisterRoutingTokenChangeEvent(routingTokenChangedHHandler_);
}

void ReliabilitySubsystem::RaRequestMessageHandler(Transport::MessageUPtr & messagePtr, RequestReceiverContextUPtr & requestReceiverContextPtr)
{
    // Currently running on the transport thread
    if (messagePtr->Action == RSMessage::GetGenerationProposal().Action)
    {
        // Process on the threadpool as this is a message for which Reliability does preprocessing 
        // and then directly calls a method on the RA
        // TODO: Refactor this so that it is a little cleaner
        WriteInfo(TraceMessageName, traceId_, "Received GenerationProposal {0}", messagePtr->MessageId);

        Common::MoveUPtr<Transport::Message> messageMover(move(messagePtr->Clone()));
        Common::MoveUPtr<RequestReceiverContext> contextMover(move(requestReceiverContextPtr));

        // Move to threadpool
        Threadpool::Post([this, messageMover, contextMover] () mutable
        {
            Transport::MessageUPtr msg = messageMover.TakeUPtr();
            RequestReceiverContextUPtr context = contextMover.TakeUPtr();
            this->RaRequestMessageHandler_Threadpool(*msg, *context);
        });
    }
    else
    {
        // Forward all other messages directly to the RA and let it handle them like the one way messages
        // The RA will enqueue them onto it's message queue and do appropriate processing
        ra_->ProcessTransportRequestResponseRequest(messagePtr, move(requestReceiverContextPtr));
    }
}

void ReliabilitySubsystem::RaRequestMessageHandler_Threadpool(Transport::Message & message, RequestReceiverContext & requestReceiverContext)
{
    ASSERT_IF(message.Action != RSMessage::GetGenerationProposal().Action, "Reliability subsystem only handles GenerationProposal");
    AcquireExclusiveLock acquire(lockObject_);

    // It is possible that we get get the messge before the FMM is actually Closed.
    // If so, we need to Close the FMM first.
    if (fm_ && fm_->IsReady)
    {
        ASSERT_IFNOT(
            fm_->IsMaster, "FM {0} is not the master FM.", fm_->Id);

        if (IsFMM())
        {
            ASSERT_IF(
                savedGFUM_, "{0} is the still the FMM but the saved GFUM is not null.", fm_->Id);
        }
        else
        {
            fm_->Close(true /* isStoreCloseNeeded */);
            fm_ = nullptr;
        }
    }

    MessageUPtr reply;

    bool isGfumUpload = false;
    auto generationProposalReply = ra_->ProcessGenerationProposal(message, isGfumUpload);

    if (isGfumUpload && savedGFUM_)
    {
        reply = RSMessage::GetGFUMTransfer().CreateMessage(*savedGFUM_);
        savedGFUM_ = nullptr;
    }
    else
    {
        reply = RSMessage::GetGenerationProposalReply().CreateMessage(generationProposalReply);
    }

    requestReceiverContext.Reply(move(reply));
}

void ReliabilitySubsystem::LookupTableUpdateRequestMessageHandler(Message & message)
{
    Assert::CodingError("Received unknown request message to service resolver: {0}", message.Action);
}

bool ReliabilitySubsystem::IsFMM() const
{
    return Federation.Token.Range.Contains(NodeId::MinNodeId);
}

void ReliabilitySubsystem::OnRoutingTokenChanged()
{
    AcquireExclusiveLock acquire(lockObject_);

    if (!isOpened_)
    {
        return;
    }

    if (IsFMM())
    {
        if (!fm_)
        {
            CreateFailoverManager();
            fm_->Open(EventHandler(), EventHandler());
        }
    }
    else
    {
        if (fm_)
        {
            savedGFUM_ = fm_->Close(true /* isStoreCloseNeeded */);
            fm_ = nullptr;
        }
    }

    ChangeNotificationOperationSPtr notificationToFMM = ChangeNotificationOperation::Create(*this, true);
    if (auto spFmm = lastSentChangeNotificationOperationFMM_.lock()) //cancel existing changenotificationoperation
    {
        spFmm->IsCancelled = true;
    }
    lastSentChangeNotificationOperationFMM_ = notificationToFMM;
    notificationToFMM->Send();
    
    ChangeNotificationOperationSPtr notificationToFM = ChangeNotificationOperation::Create(*this, false);
    if (auto spFm = lastSentChangeNotificationOperationFM_.lock()) //cancel existing changenotificationoperation
    {
        spFm->IsCancelled = true;
    }
    lastSentChangeNotificationOperationFM_ = notificationToFM;
    notificationToFM->Send();    
}

void ReliabilitySubsystem::CreateFailoverManager()
{
    FailoverManagerComponent::FailoverManagerConstructorParameters parameters = {0};
    parameters.Federation = federation_.shared_from_this();
    parameters.HealthClient = healthClient_;
    parameters.NodeConfig = nodeConfig_;

    fm_ = FailoverManagerComponent::FMMFactory(parameters);
}

void ReliabilitySubsystem::RegisterHostingSubsystemEvents()
{
    // Register for service type unregistered event
    auto root = Root.CreateComponentRoot();
  
    // Register for service type registered event
    hostingSubsystem_.RegisterServiceTypeRegisteredEventHandler(
        [this, root] (ServiceTypeRegistrationEventArgs const& eventArgs)
        {
            this->ServiceTypeRegisteredHandler(eventArgs);
        });

    // Register for service type disabled event
    hostingSubsystem_.RegisterServiceTypeDisabledEventHandler(
        [this, root] (ServiceTypeStatusEventArgs const & eventArgs)
        {
            this->ServiceTypeDisabledHandler(eventArgs);
        });

    // Register for service type enabled event
    hostingSubsystem_.RegisterServiceTypeEnabledEventHandler(
        [this, root] (ServiceTypeStatusEventArgs const & eventArgs)
        {
            this->ServiceTypeEnabledHandler(eventArgs);
        });

    // Register for runtime closed event
    hostingSubsystem_.RegisterRuntimeClosedEventHandler(
        [this, root] (RuntimeClosedEventArgs const & eventArgs)
        {
            this->RuntimeClosedHandler(eventArgs);
        });

     // Register for application host closed event
    hostingSubsystem_.RegisterApplicationHostClosedEventHandler(
        [this, root] (ApplicationHostClosedEventArgs const & eventArgs)
        {
            this->ApplicationHostClosedHandler(eventArgs);
        });

    // Register for sending available container images on node
    hostingSubsystem_.RegisterSendAvailableContainerImagesEventHandler(
        [this, root](SendAvailableContainerImagesEventArgs const & eventArgs)
        {
            this->SendAvailableContainerImagesHandler(eventArgs);
        });
}

void ReliabilitySubsystem::ServiceTypeDisabledHandler(Hosting2::ServiceTypeStatusEventArgs const& eventArgs)
{
    ra_->ProcessServiceTypeDisabled(
        eventArgs.ServiceTypeId,
        eventArgs.SequenceNumber,
        eventArgs.ActivationContext);
}

void ReliabilitySubsystem::ServiceTypeEnabledHandler(Hosting2::ServiceTypeStatusEventArgs const& eventArgs)
{
    ra_->ProcessServiceTypeEnabled(
        eventArgs.ServiceTypeId,
        eventArgs.SequenceNumber,
        eventArgs.ActivationContext);
}

void ReliabilitySubsystem::ServiceTypeRegisteredHandler(ServiceTypeRegistrationEventArgs const& eventArgs)
{
    ra_->ProcessServiceTypeRegistered(eventArgs.Registration);
}

void ReliabilitySubsystem::RuntimeClosedHandler(Hosting2::RuntimeClosedEventArgs const& eventArgs)
{
    ra_->ProcessRuntimeClosed(eventArgs.HostId, eventArgs.RuntimeId);
}

void ReliabilitySubsystem::ApplicationHostClosedHandler(Hosting2::ApplicationHostClosedEventArgs const& eventArgs)
{
    ra_->ProcessAppHostClosed(eventArgs.HostId, eventArgs.ActivityDescription);
}

void ReliabilitySubsystem::SendAvailableContainerImagesHandler(SendAvailableContainerImagesEventArgs const& eventArgs)
{
    WriteInfo(TraceLifeCycle, "Available container images on node {0}: {1}",
        eventArgs.NodeId,
        eventArgs.AvailableImages);

    AvailableContainerImagesMessageBody msgBody(
        eventArgs.NodeId,
        eventArgs.AvailableImages);

    Transport::MessageUPtr msg = RSMessage::GetAvailableContainerImages().CreateMessage<AvailableContainerImagesMessageBody>(move(msgBody));
    this->federationWrapper_.SendToFM(move(msg));
}

void ReliabilitySubsystem::ProcessRGMetrics(std::wstring const& resourceName, uint64 resourceCapacity)
{
    auto itResourceCapacity = nodeCapacityMap_.find(resourceName);
    if (itResourceCapacity == nodeCapacityMap_.end())
    {
        // if user didn't specify capacities in the cluster manifest
        // read real capacities from LRM and insert into the map
        nodeCapacityMap_.insert(make_pair(resourceName, static_cast<uint>(resourceCapacity)));
        WriteInfo(TraceLifeCycle,
            "Refreshed node capacities - inserted metric {0} -  capacity : {1}",
            resourceName, 
            resourceCapacity);
    }
    else
    {
        // otherwise, refresh capacities
        WriteInfo(TraceLifeCycle, 
            "Refreshed node capacities - changed value for metric {0} (oldCapacity : {1} - newCapacity : {2})",
            resourceName,
            itResourceCapacity->second, 
            resourceCapacity);
        itResourceCapacity->second = static_cast<uint>(resourceCapacity);
    }
}

void ReliabilitySubsystem::RefreshNodeCapacity()
{
    //read node capacities from LRM
    uint64 cpuNodeCapacity = hostingSubsystem_.GetResourceNodeCapacity(*ServiceModel::Constants::SystemMetricNameCpuCores);
    uint64 memoryNodeCapacity = hostingSubsystem_.GetResourceNodeCapacity(*ServiceModel::Constants::SystemMetricNameMemoryInMB);

    // insert or correct them in the node capacity map
    if (cpuNodeCapacity != UINT64_MAX)
    {
        ProcessRGMetrics(*ServiceModel::Constants::SystemMetricNameCpuCores, cpuNodeCapacity);
    }

    if (memoryNodeCapacity != UINT64_MAX)
    {
        ProcessRGMetrics(*ServiceModel::Constants::SystemMetricNameMemoryInMB, memoryNodeCapacity);
    }
}

void ReliabilitySubsystem::SendNodeUpToFM()
{
    // Get service packages
    nodeUpOperationFactory_->CreateAndSendNodeUpToFM(*this);
}

void ReliabilitySubsystem::SendNodeUpToFMM()
{
    nodeUpOperationFactory_->CreateAndSendNodeUpToFmm(*this);
}

void ReliabilitySubsystem::UploadLFUM(
    GenerationNumber const & generation,
    vector<FailoverUnitInfo> && failoverUnitInfos,
    bool anyReplicaFound)
{
    UploadLFUMHelper(generation, move(failoverUnitInfos), false, anyReplicaFound);
}

void ReliabilitySubsystem::UploadLFUMForFMReplicaSet(
    GenerationNumber const & generation,
    vector<FailoverUnitInfo> && failoverUnitInfos,
    bool anyReplicaFound)
{
    ASSERT_IF(failoverUnitInfos.size() > 1, "Should either be empty or only contain FM Replica");
    ASSERT_IF(!failoverUnitInfos.empty() && failoverUnitInfos[0].FailoverUnitId.Guid != Constants::FMServiceGuid, "Should contain only FM service guid");
    UploadLFUMHelper(generation, move(failoverUnitInfos), true, anyReplicaFound);
}

void ReliabilitySubsystem::UploadLFUMHelper(
    GenerationNumber const & generation,
    vector<FailoverUnitInfo> && failoverUnitInfos,
    bool isToFMM,
    bool anyReplicaFound)
{
    NodeDescription node(
        NodeConfig->NodeVersion, 
        NodeUpgradeDomainId,
        NodeFaultDomainIds,
        NodePropertyMap,
        NodeCapacityRatioMap,
        NodeCapacityMap,
        NodeConfig->InstanceName,
        NodeConfig->NodeType,
        TcpTransportUtility::ParseHostString(NodeConfig->NodeAddress),
        TcpTransportUtility::ParsePortString(NodeConfig->NodeAddress),
        NodeConfig->HttpGatewayPort());

    LFUMUploadOperationSPtr operation = make_shared<LFUMUploadOperation>(
        *this,
        generation,
        move(node),
        move(failoverUnitInfos),
        isToFMM,
        anyReplicaFound);

    operation->Send();
}

SecuritySettings const& ReliabilitySubsystem::get_securitySettings() const
{
    AcquireReadLock acquire(lockObject_);
    return securitySettings_;
}

ErrorCode ReliabilitySubsystem::SetSecurity(Transport::SecuritySettings const& value)
{
    {
        AcquireExclusiveLock acquire(lockObject_);
        this->securitySettings_ = value;

        if (!isOpened_)
        {
            return ErrorCodeValue::ObjectClosed;
        }
    }
    
    if (serviceFactorySPtr_)
    {
        return serviceFactorySPtr_->UpdateReplicatorSecuritySettings(value);
    }    

    return ErrorCodeValue::Success;
}

HHandler ReliabilitySubsystem::RegisterFailoverManagerReadyEvent(EventHandler const & handler)
{
    return fmReadyEvent_.Add(handler);
}

bool ReliabilitySubsystem::UnRegisterFailoverManagerReadyEvent(HHandler hHandler)
{
    return fmReadyEvent_.Remove(hHandler);
}

HHandler ReliabilitySubsystem::RegisterFailoverManagerFailedEvent(EventHandler const & handler)
{
    return fmFailedEvent_.Add(handler);
}

bool ReliabilitySubsystem::UnRegisterFailoverManagerFailedEvent(HHandler hHandler)
{
    return fmFailedEvent_.Remove(hHandler);
}

void ReliabilitySubsystem::OnFailoverManagerReady()
{
    fmReadyEvent_.Fire(EventArgs(), true);
}

void ReliabilitySubsystem::OnFailoverManagerFailed()
{
    fmFailedEvent_.Fire(EventArgs());
}

void ReliabilitySubsystem::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{0}", Instance);
}

// Test methods

FailoverManagerComponent::IFailoverManagerSPtr ReliabilitySubsystem::Test_GetFailoverManager() const
{
    {
        AcquireReadLock acquire(lockObject_);
        if (!isOpened_)
        {
            return nullptr;
        }
    }

    return serviceFactorySPtr_ != nullptr ? serviceFactorySPtr_->Test_GetFailoverManager() : nullptr;
}

FailoverManagerComponent::IFailoverManagerSPtr ReliabilitySubsystem::Test_GetFailoverManagerMaster() const
{
    AcquireExclusiveLock acquire(lockObject_);
    return fm_;
}

ComponentRootWPtr ReliabilitySubsystem::Test_GetFailoverManagerService() const
{
    {
        AcquireReadLock acquire(lockObject_);
        if (!isOpened_)
        {
            return ComponentRootWPtr();
        }
    }

    return serviceFactorySPtr_ != nullptr ? serviceFactorySPtr_->Test_GetFailoverManagerService() : ComponentRootWPtr();
}

bool ReliabilitySubsystem::Test_IsFailoverManagerReady() const
{    
    auto fm = Test_GetFailoverManager();
    return fm != nullptr ? fm->IsReady : false;
}

Store::ILocalStore & ReliabilitySubsystem::Test_GetLocalStore() 
{
    return ra_->Test_GetLocalStore();
}

NodeDescription ReliabilitySubsystem::GetNodeDescription() const
{
    ULONG clusterConnectionPort = TcpTransportUtility::ParsePortString(nodeConfig_->NodeAddress);

    return NodeDescription(
        nodeConfig_->NodeVersion,
        NodeUpgradeDomainId,
        NodeFaultDomainIds,
        NodePropertyMap,
        NodeCapacityRatioMap,
        NodeCapacityMap,
        nodeConfig_->InstanceName,
        nodeConfig_->NodeType,
        TcpTransportUtility::ParseHostString(nodeConfig_->NodeAddress),
        clusterConnectionPort,
        nodeConfig_->HttpGatewayPort());
}

void ReliabilitySubsystem::ProcessNodeUpAck(Transport::MessageUPtr && nodeUpReply, bool isFromFMM)
{
    ra_->ProcessNodeUpAck(move(nodeUpReply), isFromFMM);
}

void ReliabilitySubsystem::ProcessLFUMUploadReply(Reliability::GenerationHeader const & generationHeader)
{
    ra_->ProcessLFUMUploadReply(generationHeader);
}

void ReliabilitySubsystem::PopulateMetricsAdjustRG(std::map<std::wstring, uint> & capacitiesAdjust, std::map<std::wstring, uint> const & capacitiesBase) const
{
    for (auto itNodeCapacities = capacitiesBase.begin(); itNodeCapacities != capacitiesBase.end(); ++itNodeCapacities)
    {
        if (StringUtility::AreEqualPrefixPartCaseInsensitive(itNodeCapacities->first, *ServiceModel::Constants::SystemMetricNameCpuCores, L':'))
        {
            capacitiesAdjust[*ServiceModel::Constants::SystemMetricNameCpuCores] = itNodeCapacities->second;
        }
        else if (StringUtility::AreEqualPrefixPartCaseInsensitive(itNodeCapacities->first, *ServiceModel::Constants::SystemMetricNameMemoryInMB, L':'))
        {
            capacitiesAdjust[*ServiceModel::Constants::SystemMetricNameMemoryInMB] = itNodeCapacities->second;
        }
        else
        {
            capacitiesAdjust[itNodeCapacities->first] = itNodeCapacities->second;
        }
    }
}
