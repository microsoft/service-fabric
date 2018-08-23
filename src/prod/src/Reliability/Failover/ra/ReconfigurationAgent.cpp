// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "Diagnostics.EventWriterImpl.h"
#include "Reliability/Failover/ra/ResourceMonitor.ResourceComponent.h"

using namespace std;

using namespace Common;
using namespace Hosting2;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponent::Query;
using namespace ServiceModel;
using namespace ClientServerTransport;
using namespace Infrastructure;
using namespace ReconfigurationAgentComponent::Communication;
using namespace MessageRetry;

Federation::NodeInstance ReconfigurationAgent::InvalidNode = Federation::NodeInstance();
wstring const DefaultActivityId(L"_DefaultActivityId_");

namespace
{
    StringLiteral const RANotReady("not ready");

    StringLiteral const MessageQueueFullOnIpcReceiveDropTag("Enqueue into message queue failed for IPC message");
    StringLiteral const MessageQueueFullOnTcpReceiveDropTag("Enqueue into message queue failed for TCP message");
    StringLiteral const MessageQueueFullOnRequestResponseReceiveDropTag("Enqueue into message queue failed for Request/Response message");
    StringLiteral const RANotOpenOnIpcReceiveDropTag("not open on IPC message receive");
    StringLiteral const RANotOpenOnTcpReceiveDropTag("not open on TCP message receive");
    StringLiteral const RANotOpenOnRequestResponseReceiveDropTag("not open on Request/Response receive");
    StringLiteral const RANotOpenOnIpcProcessDropTag("not open on IPC message processing");
    StringLiteral const RANotOpenOnTcpProcessDropTag("not open on TCP message processing");
    StringLiteral const RANotOpenOnRequestResponseProcessDropTag("not open on Request/Response processing");
    StringLiteral const UnknownMessage("unknown message");

    wstring const StateCleanupWorkManagerName(L"StateCleanup");
    wstring const ReconfigurationMessageRetryWorkManagerName(L"MessageRetry");
    wstring const ReplicaCloseMessageRetryWorkManagerName(L"ReplicaClose");
    wstring const ReplicaOpenMessageRetryWorkManagerName(L"ReplicaOpen");
    wstring const UpdateServiceDescriptionMessageRetryWorkManagerName(L"UpdateServiceDescription");

    StringLiteral const FMMSource("FMM");
    StringLiteral const FMSource("FM");

    StringLiteral const GenerationType("Generation");
}

#pragma region Utility

template<typename T>
void CloseIfNotNull(std::unique_ptr<T> const & obj)
{
    if (obj != nullptr)
    {
        obj->Close();
    }
}

template<typename T>
void CloseIfNotNull(std::shared_ptr<T> const & obj)
{
    if (obj != nullptr)
    {
        obj->Close();
    }
}

#pragma endregion

#pragma warning(push)
#pragma warning(disable: 4355) // this used in base member initialization list
ReconfigurationAgent::ReconfigurationAgent(
    ReconfigurationAgentConstructorParameters const & params,
    IRaToRapTransportUPtr && rapTransport,
    Storage::KeyValueStoreFactory::Parameters const & lfumStoreParameters,
    Health::HealthSubsystemWrapperUPtr && health,
    FailoverConfig & config,
    std::shared_ptr<Infrastructure::EntityPreCommitNotificationSink<FailoverUnit>> const & ftPreCommitNotificationSink,
    ThreadpoolFactory threadpoolFactory,
    Diagnostics::IEventWriterUPtr && eventWriter) :
    Common::RootedObject(*params.Root),
    reliability_(params.ReliabilityWrapper),
    federationWrapper_(params.FederationWrapper),
    rapTransport_(move(rapTransport)),
    hostingSubsystem_(*params.Hosting),
    state_(*this),
    upgradeTableClosed_(false), nodeInstanceIdStr_(params.FederationWrapper->InstanceIdStr),
    nodeInstance_(params.FederationWrapper->Instance),
    lfumStoreParameters_(lfumStoreParameters),
    health_(move(health)),
    generationStateManager_(*this),
    failoverConfig_(config),
    clock_(move(params.Clock)),
    processTerminationService_(params.ProcessTerminationService),
    ftPreCommitNotificationSink_(ftPreCommitNotificationSink),
    fmTransport_(params.FederationWrapper),
    messageHandler_(*this),
    threadpoolFactory_(threadpoolFactory),
    eventWriter_(move(eventWriter))
{
    ASSERT_IF(clock_ == nullptr, "Clock cannot be null");
    ASSERT_IF(processTerminationService_ == nullptr, "processTerminationService_ can't be null");
}
#pragma warning(pop)

ReconfigurationAgent::~ReconfigurationAgent()
{
}

IReconfigurationAgentUPtr Reliability::ReconfigurationAgentComponent::ReconfigurationAgentFactory(ReconfigurationAgentConstructorParameters & parameters)
{
    auto rapTransportWrapper = make_unique<RaToRapTransport>(*parameters.IpcServer);
    
    auto & config = FailoverConfig::GetConfig();

    Health::HealthSubsystemWrapperUPtr health = make_unique<Health::HealthSubsystemWrapper>(
        parameters.ReliabilityWrapper,
        *parameters.FederationWrapper,
        config.IsHealthReportingEnabled,
        parameters.Hosting->NodeName);

    Diagnostics::IEventWriterUPtr writer = make_unique<Diagnostics::EventWriterImpl>(parameters.ReliabilityWrapper->NodeConfig->InstanceName);

    if (parameters.Clock.get() == nullptr)
    {
        parameters.Clock = make_shared<Infrastructure::Clock>();
    }

    Storage::KeyValueStoreFactory::Parameters lfumStoreParameters;
    lfumStoreParameters.IsInMemory = false;
    lfumStoreParameters.IsFaultInjectionEnabled = false;
    lfumStoreParameters.InstanceSuffix = parameters.FederationWrapper->Instance.Id.ToString();
    lfumStoreParameters.StoreFactory = parameters.StoreFactory;
    lfumStoreParameters.WorkingDirectory = parameters.ReliabilityWrapper->WorkingDirectory;
    lfumStoreParameters.KtlLoggerInstance = parameters.KtlLoggerNode;

    return make_unique<ReconfigurationAgent>(
        parameters,
        move(rapTransportWrapper),
        lfumStoreParameters,
        move(health),
        config,
        make_shared<FailoverUnitPreCommitValidator>(),
        &Infrastructure::Threadpool::Factory,
        move(writer));
}

#pragma region Open/Close

#pragma region Open

ErrorCode ReconfigurationAgent::Open(NodeUpOperationFactory & factory)
{
    UpdateNodeInstanceOnOpen();

    state_.OnOpenBegin();

    entitySets_ = EntitySetCollection();

    CreatePerformanceCounters();

    RegisterIpcMessageHandler();

    CreateNodeUpAckProcessor();

    CreateJobQueueManager();

    CreateNodeState();

    CreateServiceTypeUpdateProcessor();

    CreateUpgradeMessageProcessor();

    CreateFMMessageRetryComponent();

    CreateQueryHelper();

    CreateStateCleanupWorkManager();

    CreateReconfigurationMessageRetryWorkManager();

    CreateReplicaCloseMessageRetryWorkManager();

    CreateReplicaOpenWorkManager();

    CreateUpdateServiceDescriptionMessageRetryWorkManager();
    
    CreateHostingAdapter();

    CreateResourceMonitorComponent();

    // Initialize LFUM cache
    // The FTs require all the Failover Unit Sets etc to be initialized before
    // Hence this is the last step
    ErrorCode result = InitializeLFUM(factory);

    if (!result.IsSuccess())
    {
        RAEventSource::Events->LifeCycleOpenFailed(NodeInstanceIdStr, result);

        return result;
    }

    // RA open
    state_.OnOpenComplete();

    return result;
}

void ReconfigurationAgent::UpdateNodeInstanceOnOpen()
{
    nodeInstance_ = federationWrapper_->Instance;
    nodeInstanceIdStr_ = wformatString(nodeInstance_);
}

void ReconfigurationAgent::RegisterIpcMessageHandler()
{
    auto root = Root.CreateComponentRoot();
    RAPTransport.RegisterMessageHandler(
        RAMessage::Actor, 
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & receiverContext) 
        { 
            this->ProcessTransportIpcRequest(message, move(receiverContext));
        },
        true/*dispatchOnTransportThread*/);
}

void ReconfigurationAgent::CreateNodeUpAckProcessor()
{
    nodeUpAckProcessor_ = make_unique<NodeUpAckProcessor>(*this);
}

void ReconfigurationAgent::CreateHostingAdapter()
{
    hostingAdapter_ = make_unique<Hosting::HostingAdapter>(hostingSubsystem_, *this);
}

void ReconfigurationAgent::CreateResourceMonitorComponent()
{
    resourceMonitorComponent_ = make_unique<ResourceMonitor::ResourceComponent>(*this);
}

void ReconfigurationAgent::CreateJobQueueManager()
{
    threadpool_ = threadpoolFactory_(*this);
    jobQueueManager_ = make_unique<RAJobQueueManager>(*this);
}

void ReconfigurationAgent::CreateNodeState()
{
    nodeState_ = make_unique<Node::NodeState>(*this, entitySets_, Config);
    nodeDeactivationMessageProcessor_ = make_unique<Node::NodeDeactivationMessageProcessor>(*this);
}

void ReconfigurationAgent::CreateServiceTypeUpdateProcessor()
{
    serviceTypeUpdateProcessor_ = make_unique<Node::ServiceTypeUpdateProcessor>(
        *this,
        Config.ServiceTypeUpdateStalenessCleanupIntervalEntry,
        Config.ServiceTypeUpdateStalenessEntryKeepDurationEntry);
}

void ReconfigurationAgent::CreateUpgradeMessageProcessor()
{
    upgradeMessageProcessor_ = make_unique<Upgrade::UpgradeMessageProcessor>(*this);
}

void ReconfigurationAgent::CreateFMMessageRetryComponent()
{
    FMMessageRetryComponent::Parameters parameters;
    parameters.EntitySetCollection = &entitySets_;
    parameters.MinimumIntervalBetweenWork = &Config.PerNodeMinimumIntervalBetweenMessageToFMEntry;
    parameters.RetryInterval = &Config.FMMessageRetryIntervalEntry;
    parameters.RA = this;

    {
        parameters.Target = *FailoverManagerId::Fm;
        parameters.DisplayName = nodeInstanceIdStr_ + L"_" + L"ReplicaUpFm";
        parameters.Throttle = NodeStateObj.GetMessageThrottle(parameters.Target).get();
        parameters.PendingReplicaUploadState = &NodeStateObj.GetPendingReplicaUploadStateProcessor(parameters.Target);
        fmMessageRetryComponent_ = make_unique<FMMessageRetryComponent>(parameters);
    }

    {
        parameters.Target = *FailoverManagerId::Fmm;
        parameters.DisplayName = nodeInstanceIdStr_ + L"_" + L"ReplicaUpFmm";
        parameters.Throttle = NodeStateObj.GetMessageThrottle(parameters.Target).get();
        parameters.PendingReplicaUploadState = &NodeStateObj.GetPendingReplicaUploadStateProcessor(parameters.Target);
        fmmMessageRetryComponent_ = make_unique<FMMessageRetryComponent>(parameters);
    }
}

void ReconfigurationAgent::CreateQueryHelper()
{
    queryHelper_ = make_unique<QueryHelper>(
        *this);
}

void ReconfigurationAgent::CreatePerformanceCounters()
{
    perfCounters_ = Diagnostics::RAPerformanceCounters::CreateInstance(nodeInstanceIdStr_);
}

void ReconfigurationAgent::CreateStateCleanupWorkManager()
{
    MultipleEntityBackgroundWorkManager::Parameters workParameters;

    workParameters.Id = nodeInstanceIdStr_ + L"_" + StateCleanupWorkManagerName;
    workParameters.Name = StateCleanupWorkManagerName;
    workParameters.MinIntervalBetweenWork = &Config.MinimumIntervalBetweenPeriodicStateCleanupEntry;
    workParameters.RetryInterval = &Config.PeriodicStateCleanupIntervalEntry;
    workParameters.RA = this;
    workParameters.SetIdentifier = EntitySetIdentifier(EntitySetName::StateCleanup);
    workParameters.PerformanceCounter = &PerfCounters.NumberOfCleanupPendingFTs;
    workParameters.SetCollection = &entitySets_; 
    workParameters.FactoryFunction = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
        {
            return handlerParameters.RA.FailoverUnitCleanupProcessor(handlerParameters, context);
        };

        StateCleanupJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::Default,
            *JobItemDescription::StateCleanup,
            move(JobItemContextBase()));

        return make_shared<StateCleanupJobItem>(move(jobItemParameters));
    };

    stateCleanupWorkManager_ = make_unique<MultipleEntityBackgroundWorkManager>(workParameters);
}

void ReconfigurationAgent::CreateReplicaOpenWorkManager()
{
    MultipleEntityBackgroundWorkManager::Parameters workParameters;
    workParameters.Id = nodeInstanceIdStr_ + L"_" + ReplicaOpenMessageRetryWorkManagerName;    
    workParameters.Name = ReplicaOpenMessageRetryWorkManagerName;
    workParameters.MinIntervalBetweenWork = &Config.MinimumIntervalBetweenRAPMessageRetryEntry;
    workParameters.RetryInterval = &Config.RAPMessageRetryIntervalEntry;
    workParameters.RA = this;
    workParameters.SetIdentifier = EntitySetIdentifier(EntitySetName::ReplicaOpenMessageRetry);
    workParameters.PerformanceCounter = &PerfCounters.NumberOfReplicaOpenPendingFTs;
    workParameters.SetCollection = &entitySets_;

    workParameters.FactoryFunction = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase& context)
        {
            return handlerParameters.RA.ReplicaOpenMessageRetryProcessor(handlerParameters, context);
        };

        ReplicaOpenMessageRetryJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::DefaultAndOpen,
            *JobItemDescription::ReplicaOpenMessageRetry,
            move(JobItemContextBase()));

        return make_shared<ReplicaOpenMessageRetryJobItem>(move(jobItemParameters));
    };
    
        
    replicaOpenWorkManager_ = make_unique<MultipleEntityBackgroundWorkManager>(workParameters);
}

void ReconfigurationAgent::CreateReplicaCloseMessageRetryWorkManager()
{
    MultipleEntityBackgroundWorkManager::Parameters workParameters;

    workParameters.Id = nodeInstanceIdStr_ + L"_" + ReplicaCloseMessageRetryWorkManagerName;    
    workParameters.Name = ReplicaCloseMessageRetryWorkManagerName;
    workParameters.MinIntervalBetweenWork = &Config.MinimumIntervalBetweenRAPMessageRetryEntry;
    workParameters.RetryInterval = &Config.RAPMessageRetryIntervalEntry;
    workParameters.RA = this;
    workParameters.SetIdentifier = EntitySetIdentifier(EntitySetName::ReplicaCloseMessageRetry);
    workParameters.PerformanceCounter = &PerfCounters.NumberOfReplicaClosePendingFTs;
    workParameters.SetCollection = &entitySets_;

    workParameters.FactoryFunction = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
        {
            return handlerParameters.RA.ReplicaCloseMessageRetryProcessor(handlerParameters, context);
        };

        ReplicaCloseMessageRetryJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            static_cast<JobItemCheck::Enum>(JobItemCheck::FTIsNotNull | JobItemCheck::RAIsOpenOrClosing),
            *JobItemDescription::ReplicaCloseMessageRetry,
            move(JobItemContextBase()));

        return make_shared<ReplicaCloseMessageRetryJobItem>(move(jobItemParameters));
    };

    replicaCloseMessageRetryWorkManager_ = make_unique<MultipleEntityBackgroundWorkManager>(workParameters);
}

void ReconfigurationAgent::CreateReconfigurationMessageRetryWorkManager()
{
    MultipleEntityBackgroundWorkManager::Parameters workParameters;

    workParameters.Id = nodeInstanceIdStr_ + L"_" + ReconfigurationMessageRetryWorkManagerName;
    workParameters.Name = ReconfigurationMessageRetryWorkManagerName;
    workParameters.MinIntervalBetweenWork = &Config.MinimumIntervalBetweenReconfigurationMessageRetryEntry;
    workParameters.RetryInterval = &Config.ReconfigurationMessageRetryIntervalEntry;
    workParameters.RA = this;
    workParameters.SetIdentifier = EntitySetIdentifier(EntitySetName::ReconfigurationMessageRetry);
    workParameters.PerformanceCounter = &PerfCounters.NumberOfReconfiguringFTs;
    workParameters.SetCollection = &entitySets_;

    workParameters.FactoryFunction = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
        {
            return handlerParameters.RA.ReconfigurationMessageRetryProcessor(handlerParameters, context);
        };

        ReconfigurationMessageRetryJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::DefaultAndOpen,
            *JobItemDescription::ReconfigurationMessageRetry,
            move(JobItemContextBase()));

        return make_shared<ReconfigurationMessageRetryJobItem>(move(jobItemParameters));
    };

    reconfigurationMessageRetryWorkManager_ = make_unique<MultipleEntityBackgroundWorkManager>(workParameters);
}

void ReconfigurationAgent::CreateUpdateServiceDescriptionMessageRetryWorkManager()
{
    MultipleEntityBackgroundWorkManager::Parameters workParameters;

    workParameters.Id = nodeInstanceIdStr_ + L"_" + UpdateServiceDescriptionMessageRetryWorkManagerName;
    workParameters.Name = UpdateServiceDescriptionMessageRetryWorkManagerName;
    workParameters.MinIntervalBetweenWork = &Config.MinimumIntervalBetweenRAPMessageRetryEntry;
    workParameters.RetryInterval = &Config.RAPMessageRetryIntervalEntry;
    workParameters.RA = this;
    workParameters.SetIdentifier = EntitySetIdentifier(EntitySetName::UpdateServiceDescriptionMessageRetry);
    workParameters.PerformanceCounter = &PerfCounters.NumberOfServiceDescriptionUpdatePendingFTs;
    workParameters.SetCollection = &entitySets_;

    workParameters.FactoryFunction = [](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
    {
        auto handler = [](HandlerParameters & handlerParameters, JobItemContextBase & context)
        {
            return handlerParameters.RA.UpdateServiceDescriptionMessageRetryProcessor(handlerParameters, context);
        };

        UpdateServiceDescriptionMessageRetryJobItem::Parameters jobItemParameters(
            entry,
            work,
            handler,
            JobItemCheck::DefaultAndOpen,
            *JobItemDescription::UpdateServiceDescriptionMessageRetry,
            JobItemContextBase());

        return make_shared<UpdateServiceDescriptionMessageRetryJobItem>(move(jobItemParameters));
    };

    updateServiceDescriptionMessageRetryWorkManager_ = make_unique<MultipleEntityBackgroundWorkManager>(workParameters);
}

#pragma endregion

void ReconfigurationAgent::Close()
{
    state_.OnCloseBegin();

    CloseAllReplicas();

    state_.OnReplicasClosed();

    {
        AcquireWriteLock grab(appUpgradeslock_);

        upgradeTableClosed_ = true;
    }

    // Close the queues if they were created
    CloseIfNotNull(jobQueueManager_);
    
    CloseIfNotNull(upgradeMessageProcessor_);

    CloseIfNotNull(stateCleanupWorkManager_);

    CloseIfNotNull(reconfigurationMessageRetryWorkManager_);
    CloseIfNotNull(replicaCloseMessageRetryWorkManager_);
    CloseIfNotNull(replicaOpenWorkManager_);
    CloseIfNotNull(updateServiceDescriptionMessageRetryWorkManager_);
    CloseIfNotNull(fmMessageRetryComponent_);
    CloseIfNotNull(fmmMessageRetryComponent_);

    CloseIfNotNull(hostingAdapter_);

    CloseIfNotNull(serviceTypeUpdateProcessor_);

    CloseIfNotNull(nodeDeactivationMessageProcessor_);
    CloseIfNotNull(nodeState_);

    CloseIfNotNull(lfum_);    
    CloseIfNotNull(raStoreSPtr_);

    // Close the threadpool last
    CloseIfNotNull(threadpool_);

    state_.OnCloseComplete();
}

Store::ILocalStore & ReconfigurationAgent::Test_GetLocalStore()
{
    Storage::LocalStoreAdapter * innerStore = nullptr;

    innerStore = dynamic_cast<Storage::LocalStoreAdapter *>(raStoreSPtr_.get());
    ASSERT_IF(innerStore == nullptr, "trying to get test local store when ra is not running with local store");

    return *(innerStore->get_Test_LocalStore());
}

ErrorCode ReconfigurationAgent::InitializeLFUM(NodeUpOperationFactory & factory)
{
    auto error = Storage::KeyValueStoreFactory().TryCreate(
        lfumStoreParameters_,
        this,
        raStoreSPtr_);
    if (!error.IsSuccess())
    {
        WriteError("LifeCycle", "Failed to create store {0}", error);
        return error;
    }

    lfum_ = make_unique<LocalFailoverUnitMap>(*this, Clock, PerfCounters, ftPreCommitNotificationSink_);
    LocalStorageInitializer storageInitializer(*this, factory);
    return storageInitializer.Open();
}

void ReconfigurationAgent::CloseAllReplicas()
{
    if (Config.GracefulReplicaShutdownMaxDuration == TimeSpan::Zero)
    {
        return;
    }

    // LFUM will be uninitialized if open fails before InitializeLFUM()
    //
    if (lfum_.get() == nullptr)
    {
        return;
    }

    auto fts = lfum_->GetAllFailoverUnitEntries(false);

    ManualResetEvent ev;

    // Start the close completion check
    MultipleReplicaCloseAsyncOperation::Parameters parameters;
    parameters.Clock = ClockSPtr;
    parameters.ActivityId = L"NodeShutdown";
    parameters.Callback = [](wstring const & activityId, Infrastructure::EntityEntryBaseList const & pendingCloseFT, ReconfigurationAgent & ra)
    {
        RAEventSource::Events->LifeCycleCloseReplicaInProgress(ra.NodeInstanceIdStr, activityId, pendingCloseFT);
    };
    parameters.CloseMode = ReplicaCloseMode::Close;
    parameters.FTsToClose = move(fts);
    parameters.JobItemCheck = JobItemCheck::FTIsNotNull;
    parameters.MaxWaitBeforeTerminationEntry = &Config.GracefulReplicaShutdownMaxDurationEntry;
    parameters.MonitoringIntervalEntry = &Config.GracefulReplicaCloseCompletionCheckIntervalEntry;
    parameters.RA = this;
    parameters.TerminateReason = Hosting::TerminateServiceHostReason::NodeShutdown;

    auto op = AsyncOperation::CreateAndStart<MultipleReplicaCloseAsyncOperation>(
        move(parameters),
        [&ev](AsyncOperationSPtr const & inner)
        {
            auto result = MultipleReplicaCloseAsyncOperation::End(inner);
            TESTASSERT_IF(!result.IsSuccess(), "Close replicas during node shutdown failed");

            ev.Set();
        },
        Root.CreateAsyncOperationRoot());

    // Keep a strong ref alive for the call to cancel
    // Hold on to a weak ref in the RA (otherwise there is a circular dependency)
    auto strongRef = static_pointer_cast<MultipleReplicaCloseAsyncOperation>(op);
    closeReplicasAsyncOp_ = strongRef;

    // The entire stack is sync :( 
    // TODO: Fix this
    // Also, to safeguard against things getting stuck due to bugs (terminate api getting stuck or debuggers attaching etc)
    // wait for a max of double the duration
    auto maxWaitDuration = Config.GracefulReplicaShutdownMaxDuration + Config.GracefulReplicaShutdownMaxDuration;
    if (!ev.WaitOne(maxWaitDuration))
    {
        strongRef->Cancel();
    }
}

#pragma endregion

#pragma region Transport Thread Message handlers

void ReconfigurationAgent::ProcessTransportRequest(Transport::MessageUPtr & message, Federation::OneWayReceiverContextUPtr && context)
{
    messageHandler_.ProcessTransportRequest(message, move(context));
}

void ReconfigurationAgent::ProcessTransportIpcRequest(Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr && receiverContext)
{
    messageHandler_.ProcessTransportIpcRequest(message, move(receiverContext));
}

void ReconfigurationAgent::ProcessTransportRequestResponseRequest(Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr && receiverContext)
{
    messageHandler_.ProcessTransportRequestResponseRequest(message, move(receiverContext));
}

#pragma endregion

#pragma region Local Replica LifeCycle
void ReconfigurationAgent::CloseLocalReplica(
    Infrastructure::HandlerParameters & handlerParameters,
    ReplicaCloseMode closeMode,
    Common::ActivityDescription const & activityDescription)
{
    ASSERT_IF(closeMode == ReplicaCloseMode::Deactivate, "Use other overload");
    CloseLocalReplica(handlerParameters, closeMode, ReconfigurationAgent::InvalidNode, activityDescription);
}

void ReconfigurationAgent::CloseLocalReplica(
    Infrastructure::HandlerParameters & handlerParameters,
    ReplicaCloseMode closeMode,
    Federation::NodeInstance const & senderNode,
    Common::ActivityDescription const & activityDescription)
{
    auto & failoverUnit = handlerParameters.FailoverUnit;

    if (!failoverUnit->CanCloseLocalReplica(closeMode))
    {
        return;
    }

    failoverUnit->StartCloseLocalReplica(closeMode, senderNode, handlerParameters.ExecutionContext, activityDescription);
    if (failoverUnit->LocalReplicaClosePending.IsSet)
    {
        ProcessReplicaCloseMessageRetry(handlerParameters);
    }
    else if (failoverUnit->IsLocalReplicaClosed(closeMode))
    {
        // It is possible that the start close method actually completed the close
        // without needing a message to be sent to RAP
        // In this case RA should perform post processing that it needs
        FinishCloseLocalReplica(handlerParameters, closeMode, senderNode);
    }
}

void ReconfigurationAgent::FinishCloseLocalReplica(
    Infrastructure::HandlerParameters & handlerParameters,
    ReplicaCloseMode closeMode,
    Federation::NodeInstance const & senderNode)
{
    auto & ft = handlerParameters.FailoverUnit;
    auto & queue = handlerParameters.ActionQueue;

    TESTASSERT_IFNOT(ft->IsLocalReplicaClosed(closeMode), "Must be called after the close has completed {0}", *ft);

    if (closeMode == ReplicaCloseMode::Restart || 
       (closeMode == ReplicaCloseMode::DeactivateNode && nodeState_->GetNodeDeactivationState(ft->Owner).IsActivated))
    {
        ReopenDownReplica(handlerParameters);
    }

    if (closeMode == ReplicaCloseMode::Deactivate)
    {
        AddActionSendMessageReplyToRA(
            queue,
            RSMessage::GetDeactivateReply(),
            senderNode,
            ft->FailoverUnitDescription,
            ft->CreateDroppedReplicaDescription(NodeInstance),
            ErrorCodeValue::Success);
    }

    if (closeMode == ReplicaCloseMode::Delete)
    {
        ReplicaReplyMessageBody body(ft->FailoverUnitDescription, ft->CreateDroppedReplicaDescription(NodeInstance), ErrorCode::Success());

        ft->TransportUtility.AddSendMessageToFMAction(
            queue,
            ft->IsStateful ? RSMessage::GetDeleteReplicaReply() : RSMessage::GetRemoveInstanceReply(),
            move(body));
    }
}

bool ReconfigurationAgent::ReopenDownReplica(HandlerParameters & handlerParameters)
{
    return ReopenDownReplica(nullptr, handlerParameters);
}

bool ReconfigurationAgent::ReopenDownReplica(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    HandlerParameters & handlerParameters)
{
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    if (StateObj.IsUpgrading)
    {
        return false;
    }

	if (!nodeState_->GetNodeDeactivationState(failoverUnit->Owner).IsActivated)
	{
		return false;
	}

    if (!failoverUnit->CanReopenDownReplica())
    {
        return false;
    }

    failoverUnit->ReopenDownReplica(handlerParameters.ExecutionContext);
    if (failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        // open for previous SB replica
        ProcessReplicaOpenMessageRetry(registration, handlerParameters);
    }
    else if (failoverUnit->LocalReplicaClosePending.IsSet)
    {
        // open of previous ID replica
        ProcessReplicaCloseMessageRetry(registration, handlerParameters);
    }

    return true;
}

void ReconfigurationAgent::CreateLocalReplica(
    Infrastructure::HandlerParameters & handlerParameters,
    ReplicaMessageBody const & msgBody,
    Reliability::ReplicaRole::Enum localReplicaRole,
    ReplicaDeactivationInfo const & deactivationInfo,
    bool isRebuild)
{
    auto & failoverUnit = handlerParameters.FailoverUnit;

    // If the FT is closed and CreateLocalReplica is called it means we should reset all state to default values
    failoverUnit.EnableUpdate();

    // NOTE: It is important to use the service description from the message body
    // The FT may be Closed (as above) or have been recently created in which case the ServicePackageId is not set on the FT
    ServicePackageVersionInstance packageVersionInstance = GetServicePackageVersionInstance(failoverUnit, msgBody.ServiceDescription.Type.ServicePackageId, msgBody.ReplicaDescription.PackageVersionInstance);

    failoverUnit->StartCreateLocalReplica(
        msgBody.FailoverUnitDescription,
        msgBody.ReplicaDescription,
        msgBody.ServiceDescription,
        packageVersionInstance,
        localReplicaRole,
        deactivationInfo,
        isRebuild,
        handlerParameters.ExecutionContext);
}

void ReconfigurationAgent::DeleteLocalReplica(
    Infrastructure::HandlerParameters & handlerParameters,
    DeleteReplicaMessageBody const & msgBody)
{
    auto & failoverUnit = handlerParameters.FailoverUnit;
    TryCreateFailoverUnit(msgBody, failoverUnit);

    failoverUnit->OnReplicaUploaded(handlerParameters.ExecutionContext);

    if (failoverUnit->IsClosed)
    {
        /*
            Update the versions/instance to handle stale messages.

            Always update the epoch

            The ReplicaId and instance is updated if the replica id is higher. This can happen if the node had replica r1.
            FM placed r2 on it but before the node saw Add for r2 for some reason FM deleted r2.

            The instance id can never be higher in the FM view because it is generated by the node.

            If the instance is already deleted then it is no op
        */
        failoverUnit->UpdateInstanceForClosedReplica(
            msgBody.FailoverUnitDescription, 
            msgBody.ReplicaDescription, 
            false,
            handlerParameters.ExecutionContext);
    }

    if (!msgBody.IsForce && (failoverUnit->CurrentConfigurationEpoch > msgBody.FailoverUnitDescription.CurrentConfigurationEpoch))
    {
        return;
    }

    if (failoverUnit->LocalReplicaId != msgBody.ReplicaDescription.ReplicaId)
    {
        // The FT already existed on the node
        // Allow the delete only if the replica id matches
        // The RA may have a higher instance due to restarts but there is no need
        // to delay the delete from the FM
        return;
    }

    if (failoverUnit->IsClosed)
    {
        CloseLocalReplica(handlerParameters, ReplicaCloseMode::Delete, ActivityDescription::Empty);
    }
    else
    {
        CloseLocalReplica(handlerParameters, msgBody.IsForce ? ReplicaCloseMode::Obliterate : ReplicaCloseMode::Delete, ActivityDescription::Empty);
    }
    return;
}

bool ReconfigurationAgent::ReplicaOpenMessageRetryProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertFTIsOpen();

    return ProcessReplicaOpenMessageRetry(nullptr, handlerParameters);
}

bool ReconfigurationAgent::ProcessReplicaOpenMessageRetry(
    HandlerParameters & handlerParameters)
{
    return ProcessReplicaOpenMessageRetry(nullptr, handlerParameters);
}

bool ReconfigurationAgent::ProcessReplicaOpenMessageRetry(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    HandlerParameters & handlerParameters)
{
    auto & failoverUnit = handlerParameters.FailoverUnit;

    // Check this flag prior to capturing state
    if (!failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        return false;
    }

    /*
        It is important to check if node up ack has been processed
        FTs are not opened and hosting events are not handled prior to node up ack being processed
    */
    if (!IsReady(failoverUnit->FailoverUnitId))
    {
        return false;
    }

    // Capture state
    auto openMode = failoverUnit->OpenMode;
    auto senderNode = failoverUnit->SenderNode;
    auto replicaDescription = failoverUnit->LocalReplica.ReplicaDescription;
    auto error = failoverUnit->ProcessReplicaOpenRetry(registration, handlerParameters.ExecutionContext);
    if (!error.IsSuccess())
    {
        FinishOpenLocalReplica(failoverUnit, handlerParameters, openMode, senderNode, replicaDescription, error.ReadValue());
        return true;
    }

    // message should be sent to rap so open will complete when reply is received
    TESTASSERT_IF(!failoverUnit->LocalReplicaOpenPending.IsSet, "FT should still be opening {0}", *failoverUnit);
    return true;
}

void ReconfigurationAgent::FinishOpenLocalReplica(
    LockedFailoverUnitPtr & failoverUnit,
    HandlerParameters & handlerParameters,
    RAReplicaOpenMode::Enum openMode,
    Federation::NodeInstance const & senderNode,
    Reliability::ReplicaDescription const & localReplicaDescription,
    ErrorCodeValue::Enum error)
{
    auto & queue = handlerParameters.ActionQueue;
    TESTASSERT_IF(error == ErrorCodeValue::RAProxyStateChangedOnDataLoss, "Cannot return state changed on data loss to FM");

    if (!failoverUnit->IsStateful)
    {
        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, localReplicaDescription, error);

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            queue,
            RSMessage::GetAddInstanceReply(),
            move(body));
    }
    else if (openMode == RAReplicaOpenMode::Reopen)
    {
        // no-op
        // the order of these statements is significant -> reopening replicas should not send back add primary reply
    }
    else if (localReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary)
    {
        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, localReplicaDescription, error);

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            queue,
            RSMessage::GetAddPrimaryReply(),
            move(body));
    }
    else
    {
        TESTASSERT_IF(senderNode == ReconfigurationAgent::InvalidNode, "Sender cannot be invalid at ReplicaOpenReply for CreateIdle {0}", *failoverUnit);
        AddActionSendMessageReplyToRA(
            queue,
            RSMessage::GetCreateReplicaReply(),
            senderNode,
            failoverUnit->FailoverUnitDescription,
            localReplicaDescription,
            error);

        if (error != ErrorCodeValue::Success)
        {
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, queue);
        }
    }
}

bool ReconfigurationAgent::ReplicaCloseMessageRetryProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertHasWork();

    return ProcessReplicaCloseMessageRetry(handlerParameters);   
}

bool ReconfigurationAgent::ProcessReplicaCloseMessageRetry(
    Infrastructure::HandlerParameters & handlerParameters)
{
    return ProcessReplicaCloseMessageRetry(nullptr, handlerParameters);
}
    
bool ReconfigurationAgent::ProcessReplicaCloseMessageRetry(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    Infrastructure::HandlerParameters & handlerParameters)
{
    // Capture state
    auto & failoverUnit = handlerParameters.FailoverUnit;

    if (!failoverUnit->LocalReplicaClosePending.IsSet)
    {
        return false;
    }

    /*
        It is important to check if node up ack has been processed
        FTs are not opened and hosting events are not handled prior to node up ack being processed
    */
    if (!IsReady(failoverUnit->FailoverUnitId))
    {
        return false;
    }

    auto closeMode = failoverUnit->CloseMode;
    auto sender = failoverUnit->SenderNode;
    
    auto error = failoverUnit->ProcessReplicaCloseRetry(registration, handlerParameters.ExecutionContext);

    if (!error.IsSuccess())
    {
        FinishCloseLocalReplica(handlerParameters, closeMode, sender);
        return true;
    }

    TESTASSERT_IF(!failoverUnit->LocalReplicaClosePending.IsSet, "FT must continue closing {0}", *failoverUnit);
    return true;
}

#pragma endregion

ErrorCode ReconfigurationAgent::GetServiceTypeRegistration(
    FailoverUnitId const & ftId,
    ServiceDescription const & serviceDescription,
    ServiceTypeRegistrationSPtr & registration)
{
    ServicePackageVersionInstance const & inPackageVersionInstance = serviceDescription.PackageVersionInstance;
    ServiceTypeIdentifier const & typeId = serviceDescription.Type;
    uint64 sequenceNumber = 0;

    auto error = 
        hostingSubsystem_.FindServiceTypeRegistration(
            VersionedServiceTypeIdentifier(typeId, inPackageVersionInstance),
            serviceDescription,
            serviceDescription.CreateActivationContext(ftId.Guid),
            sequenceNumber,
            registration);

    RAEventSource::Events->HostingFindServiceTypeRegistrationCompleted(
        NodeInstanceIdStr,
        typeId,
        error,
        inPackageVersionInstance,
        registration ? registration->servicePackageVersionInstance : ServicePackageVersionInstance::Invalid,
        registration ? registration->HostId : wstring(),
        registration ? registration->RuntimeId : wstring());

    if (error.IsError(ErrorCodeValue::HostingServiceTypeRegistrationVersionMismatch))
    {
        /*
            Find was successful, but version numbers didnt match
            This can happen in case the FM asked to create a replica in a higher version (say V3)
            And another replica in V2 is running on the node
            At this time use V2 as the version and start the replica in V2
            The replica will eventually be upgraded when the FM comes and upgrades this node
        */
        ServicePackageVersionInstance const & outpackageVersionInstance = registration->servicePackageVersionInstance;

        AcquireReadLock grab(appUpgradeslock_);

        auto it = upgradeVersions_.find(serviceDescription.Type.ServicePackageId);
        if (it == upgradeVersions_.end() || it->second.InstanceId <= outpackageVersionInstance.InstanceId)
        {
            error = ErrorCode::Success();
        }
    }

    return error;
}

MessageRetry::FMMessageRetryComponent & ReconfigurationAgent::GetFMMessageRetryComponent(Reliability::FailoverManagerId const & target)
{
    return target.IsFmm ? *fmmMessageRetryComponent_ : *fmMessageRetryComponent_;
}

bool ReconfigurationAgent::UpdateServiceDescriptionMessageRetryProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertFTIsOpen();

    auto & ft = handlerParameters.FailoverUnit;
    auto & queue = handlerParameters.ActionQueue;

    if (ft->LocalReplicaServiceDescriptionUpdatePending.IsSet)
    {
        ReconfigurationAgent::AddActionSendMessageToRAProxy(
            queue,
            RAMessage::GetUpdateServiceDescription(),
            ft->ServiceTypeRegistration,
            ft->FailoverUnitDescription,
            ft->LocalReplica.ReplicaDescription,
            ft->ServiceDescription);
    }

    return true;
}

void ReconfigurationAgent::ProcessServiceTypeRegistered(ServiceTypeRegistrationSPtr const & registration)
{
    hostingAdapter_->EventHandler.ProcessServiceTypeRegistered(registration);
}

void ReconfigurationAgent::ProcessServiceTypeDisabled(
    ServiceModel::ServiceTypeIdentifier const & serviceTypeId, 
    uint64 sequenceNumber,
    ServiceModel::ServicePackageActivationContext const & activationContext)
{
    serviceTypeUpdateProcessor_->ProcessServiceTypeDisabled(serviceTypeId, sequenceNumber, activationContext);
}

void ReconfigurationAgent::ProcessServiceTypeEnabled(
    ServiceModel::ServiceTypeIdentifier const & serviceTypeId, 
    uint64 sequenceNumber,
    ServiceModel::ServicePackageActivationContext const & activationContext)
{
    serviceTypeUpdateProcessor_->ProcessServiceTypeEnabled(serviceTypeId, sequenceNumber, activationContext);
}

bool ReconfigurationAgent::ServiceTypeRegisteredProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    handlerParameters.AssertFTIsOpen();
    handlerParameters.AssertHasWork();

    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;
    Hosting2::ServiceTypeRegistrationSPtr const & registration = handlerParameters.Work->GetInput<ServiceTypeRegisteredInput>().Registration;

    if (failoverUnit->ServiceDescription.Type != registration->ServiceTypeId ||
        failoverUnit->LocalReplica.PackageVersionInstance != registration->servicePackageVersionInstance)
    {
        return false;
    }

    /*
        Handle isolation contexts
        If the service description says that the isolation context is exclusive then service type registration handler
        must enqueue the job item for that partition and hence this can be asserted

        The other condition cannot be asserted. It is possible that the service type registration is for ths shared type
        but this partition is exclusive. This is because at the time the job item is created this partition may not
        have been committed to disk. The job item must acquire a write lock to examine the service description
    */
    if (registration->ActivationContext.IsExclusive && failoverUnit->ServiceDescription.PackageActivationMode != ServicePackageActivationMode::ExclusiveProcess)
    {
        Assert::TestAssert("Partition. STR {0} is exclusive and partition is not\r\nFT: {1}", *registration, *failoverUnit);
        return false;
    }
    else if (!registration->ActivationContext.IsExclusive && failoverUnit->ServiceDescription.PackageActivationMode == ServicePackageActivationMode::ExclusiveProcess)
    {
        return false;
    }

    if (!failoverUnit->LocalReplica.IsUp)
    {
        return ReopenDownReplica(registration, handlerParameters);
    }
    else if (failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        return ProcessReplicaOpenMessageRetry(registration, handlerParameters);
    }
    else if (failoverUnit->LocalReplicaClosePending.IsSet)
    {
        return ProcessReplicaCloseMessageRetry(registration, handlerParameters);
    }

    return false;
}

void ReconfigurationAgent::ProcessAppHostClosed(wstring const & hostId, Common::ActivityDescription const & activityDescription)
{
    HostingAdapterObj.EventHandler.ProcessAppHostClosed(hostId, activityDescription);
}

bool ReconfigurationAgent::AppHostClosedProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    handlerParameters.AssertHasWork();
    handlerParameters.AssertFTExists();

    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;
    AppHostClosedInput const & input = handlerParameters.Work->GetInput<AppHostClosedInput>();

    if (failoverUnit->AppHostId != input.HostId)
    {
        // this ft is not affected by this event
        return false;
    }
    
    /*
        The app host down should be considered retryable only in certain scenarios.

        If the node is upgrading or app is upgrading or node is deactivating or node is closing then consider host down as terminal. This
        is because these code paths require the replica to go down anyway so shortcircuit any retries here.

        If it is not persisted then any drop is terminal - there is no need to retry because the state has been lost.

        For opening replicas always retry to reduce load on fm. If the node is upgrading/deactivating, that logic will transition it to closing
    */
    bool isAppUpgrading = upgradeMessageProcessor_->IsUpgrading(failoverUnit->ServiceDescription.ApplicationId.ToString());
    bool isActivated = nodeState_->GetNodeDeactivationState(failoverUnit->Owner).IsActivated;
    bool isAppHostDownRetryable = !isAppUpgrading && !StateObj.IsUpgrading && !StateObj.IsClosing && isActivated;

    auto closeMode = ReplicaCloseMode::AppHostDown;
    
    if (failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        ReplicaOpenReplyHandlerHelper(failoverUnit, failoverUnit->LocalReplica.ReplicaDescription, TransitionErrorCode(Common::ErrorCode(ErrorCodeValue::Enum::ApplicationHostCrash)), handlerParameters);
    }
    else if (isAppHostDownRetryable && failoverUnit->ShouldRetryCloseReplica())
    {
        ReplicaCloseReplyHandlerHelper(failoverUnit, TransitionErrorCode(Common::ErrorCode(ErrorCodeValue::Enum::ApplicationHostCrash)), handlerParameters);
    }
    else
    {
        CloseLocalReplica(handlerParameters, closeMode, input.ActivityDescription);
    }

    return true;
}

void ReconfigurationAgent::ProcessRuntimeClosed(std::wstring const & hostId, std::wstring const & runtimeId)
{
    HostingAdapterObj.EventHandler.ProcessRuntimeClosed(hostId, runtimeId);
}

bool ReconfigurationAgent::RuntimeClosedProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    handlerParameters.AssertFTExists();
    handlerParameters.AssertFTIsOpen();

    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;
    RuntimeClosedInput const & input = handlerParameters.Work->GetInput<RuntimeClosedInput>();

    if (failoverUnit->AppHostId != input.HostId || failoverUnit->RuntimeId != input.RuntimeId)
    {
        return false;
    }

    auto closeMode = ReplicaCloseMode::Close;
    if (!failoverUnit->CanCloseLocalReplica(closeMode) || failoverUnit->IsLocalReplicaClosed(closeMode))
    {
        return false;
    }

    CloseLocalReplica(handlerParameters, closeMode, ActivityDescription::Empty);
    return true;
}

void ReconfigurationAgent::ProcessRequestResponseRequest(
    IMessageMetadata const * metadata,
    MessageUPtr && requestPtr, 
    Federation::RequestReceiverContextUPtr && context)
{
    messageHandler_.ProcessRequestResponseRequest(metadata, move(requestPtr), move(context));
}

void ReconfigurationAgent::ClientReportFaultRequestHandler(MessageUPtr && requestPtr, Federation::RequestReceiverContextUPtr && context)
{
    Message & request = *requestPtr;

    ReportFaultRequestMessageBody body;
    if (!request.GetBody(body))
    {
        federationWrapper_->CompleteRequestReceiverContext(*context, ErrorCodeValue::InvalidMessage);
        return;
    }

    auto activityId = RSMessage::GetActivityId(request);
    
    // Force flag is not supported for RF transient
    if (body.IsForce && body.FaultType == FaultType::Transient)
    {
        CompleteReportFaultRequest(activityId, *context, ErrorCode(ErrorCodeValue::ForceNotSupportedForReplicaOperation, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_TRANSIENT_WITH_FORCE)));
        return;
    }

    auto entry = LocalFailoverUnitMapObj.GetEntry(FailoverUnitId(body.PartitionId));
    if (entry == nullptr)
    {
        CompleteReportFaultRequest
        (activityId, *context, ErrorCode(ErrorCodeValue::REReplicaDoesNotExist, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_FOR_NON_EXISTENT_FAILOVER_UNIT)));
        return;
    }

    // TODO: The RA should move to using the same activity id as the rest of the product
    wstring jobItemActivityId = wformatString(activityId);
    ClientReportFaultJobItemContext jobItemContext(move(body), move(context), activityId);

    ClientReportFaultJobItem::Parameters parameters(
        entry,
        move(jobItemActivityId),
        [] (HandlerParameters & handlerParameters, ClientReportFaultJobItemContext & inner)
        {
            return handlerParameters.RA.ClientReportFaultRequestProcessor(handlerParameters, inner);
        },
        JobItemCheck::RAIsOpen,
        *JobItemDescription::ClientReportFault,
        move(jobItemContext));

    auto jobItem = make_shared<ClientReportFaultJobItem>(move(parameters));
            
    JobQueueManager.ScheduleJobItem(move(jobItem));

    RAEventSource::Events->ReportFaultProcessingBegin(
        Guid::NewGuid(),
        this->NodeInstanceIdStr,
        body.PartitionId,
        body.ReplicaId,
        body.FaultType,
        body.IsForce,
        body.ActivityDescription);
}

void ReconfigurationAgent::ProcessRequest(
    IMessageMetadata const * metadata,
    MessageUPtr && requestPtr, 
    Federation::OneWayReceiverContextUPtr && context)
{
    messageHandler_.ProcessRequest(metadata, move(requestPtr), move(context));
}

void ReconfigurationAgent::ProcessNodeUpdateServiceRequest(Message & request)
{    
    shared_ptr<NodeUpdateServiceRequestMessageBody> body = make_shared<NodeUpdateServiceRequestMessageBody>();
    if (!TryGetMessageBody(NodeInstanceIdStr, request, *body))
    {
        return;
    }
    
    wstring activityId = wformatString(request.MessageId);
    
    auto work = make_shared<MultipleEntityWork>(
        activityId, 
        [body] (MultipleEntityWork & work, ReconfigurationAgent & ra)
        {
            ra.OnNodeUpdateServiceDescriptionProcessingComplete(work, *body);
        });

    auto handler = [body](HandlerParameters & handlerParameters, NodeUpdateServiceJobItemContext & context)
    {
        return handlerParameters.RA.NodeUpdateServiceDescriptionProcessor(*body, handlerParameters, context);
    };

    auto fts = LocalFailoverUnitMapObj.GetFailoverUnitEntries(body->Owner);

    jobQueueManager_->CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        fts,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            NodeUpdateServiceJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                JobItemCheck::Default,
                *JobItemDescription::NodeUpdateService,
                move(NodeUpdateServiceJobItemContext()));

            return make_shared<NodeUpdateServiceJobItem>(move(jobItemParameters));
        });
}

void ReconfigurationAgent::ServiceTypeNotificationReplyHandler(Transport::Message & request)
{
    ServiceTypeNotificationReplyMessageBody body;
    if (!TryGetMessageBody(NodeInstanceIdStr, request, body))
    {
        return;
    }

    HostingAdapterObj.ProcessServiceTypeNotificationReply(body);
}

void ReconfigurationAgent::ProcessNodeFabricUpgradeHandler(Message & request)
{
    FabricUpgradeSpecification newVersion;
    if (!TryGetMessageBody(NodeInstanceIdStr, request, newVersion))
    {
        return;
    }

    ProcessNodeFabricUpgradeHandler(wformatString(request.MessageId), newVersion);
}

bool ReconfigurationAgent::CanProcessNodeFabricUpgrade(wstring const & activityId, ServiceModel::FabricUpgradeSpecification const & newVersion)
{
    if (!state_.IsReady)
    {
        return false;
    }

    auto nodeInstance = Reliability.NodeConfig->NodeVersion;
    FabricVersionInstance incoming(newVersion.Version, newVersion.InstanceId);

    Upgrade::FabricUpgradeStalenessChecker checker;
    auto stalenessCheckResult = checker.CheckFabricUpgradeAtUpgradeMessage(nodeInstance, incoming);

    switch (stalenessCheckResult)
    {
    case Upgrade::FabricUpgradeStalenessCheckResult::UpgradeNotRequired:
        {
            //send the reply as the message is stale or upgrade has already been performed
            NodeFabricUpgradeReplyMessageBody body(incoming);
            FMTransportObj.SendFabricUpgradeReplyMessage(activityId, body);
            return false;
        }

    case Upgrade::FabricUpgradeStalenessCheckResult::UpgradeRequired:
        return true;

    case Upgrade::FabricUpgradeStalenessCheckResult::Assert:
        Assert::TestAssert("Unexpected upgrade message. NodeVersion = {0}. Incoming = {1}", Reliability.NodeConfig->NodeVersion, newVersion);
        WriteInfo("Ignore", "RA ignored unexpected upgrade message NodeVersion = {0}. Incoming = {1}", Reliability.NodeConfig->NodeVersion, newVersion);
        return false;

    default:
        Assert::CodingError("Unexpected return value {0}", static_cast<int>(stalenessCheckResult));
    }
}

void ReconfigurationAgent::ProcessNodeFabricUpgradeHandler(std::wstring const & activityId, ServiceModel::FabricUpgradeSpecification const & newVersion)
{
    if (!CanProcessNodeFabricUpgrade(activityId, newVersion))
    {
        return;
    }

    auto upgrade = make_unique<Upgrade::FabricUpgrade>(activityId, Reliability.NodeConfig->NodeVersion.Version.CodeVersion, newVersion, false, *this);
    auto upgradeStateMachine = Upgrade::UpgradeStateMachine::Create(move(upgrade), *this);

    upgradeMessageProcessor_->ProcessUpgradeMessage(
        *Reliability::Constants::FabricApplicationId,
        upgradeStateMachine);
}

void ReconfigurationAgent::ProcessCancelFabricUpgradeHandler(Message & request)
{
    if (!state_.IsReady)
    {
        return;
    }

    FabricUpgradeSpecification version;
    if (!TryGetMessageBody(NodeInstanceIdStr, request, version))
    {
        return;
    }

    auto context = Upgrade::CancelFabricUpgradeContext::Create(move(version), *this);
    upgradeMessageProcessor_->ProcessCancelUpgradeMessage(
        *Reliability::Constants::FabricApplicationId,
        context);
}

void ReconfigurationAgent::ProcessNodeUpgradeHandler(Message & request)
{
    if (!state_.IsReady)
    {
        return;
    }

    // It is important for this lock to guard the entire check from ProcessUpgradeMessage to the upgradeVersions_ being updated
    // This is because we want to atomically enumerate the LFUM for FTIds as well as set the highest known package version for an upgrade
    // Otherwise it is possible that a Create between these two steps may be missed by the upgrade enumeration and use an older version
    AcquireWriteLock grab(appUpgradeslock_);

    if (upgradeTableClosed_)
    {
        return;
    }

    UpgradeDescription upgradeDesc;
    if (!TryGetMessageBody(NodeInstanceIdStr, request, upgradeDesc))
    {
        return;
    }

    ApplicationIdentifier appId = upgradeDesc.ApplicationId;    

    wstring activityId = wformatString(request.MessageId);
    auto upgrade = make_unique<Upgrade::ApplicationUpgrade>(move(activityId), upgradeDesc.Specification, *this);
    auto sm = Upgrade::UpgradeStateMachine::Create(move(upgrade), *this);

    if (upgradeMessageProcessor_->ProcessUpgradeMessage(appId.ToString(), sm))
    {
        for (auto it = upgradeDesc.Specification.PackageUpgrades.begin(); it != upgradeDesc.Specification.PackageUpgrades.end(); ++it)
        {
            auto packageUpgrade = *it;

            ServicePackageIdentifier packageId(upgradeDesc.Specification.ApplicationId, packageUpgrade.ServicePackageName);

            ServicePackageVersionInstance versionInstance;
            bool found = upgradeDesc.Specification.GetUpgradeVersionForPackage(packageUpgrade.ServicePackageName, versionInstance);
            ASSERT_IF(!found, "version not found for {0} in {1}", packageUpgrade.ServicePackageName, upgradeDesc.Specification);

            upgradeVersions_[packageId] = versionInstance;

            RAEventSource::Events->ApplicationUpgradePrintServiceTypeStatus(nodeInstanceIdStr_, versionInstance, packageId.ToString());
        }
    }
}

void ReconfigurationAgent::ProcessCancelApplicationUpgradeHandler(Message & request)
{
    if (!state_.IsReady)
    {
        return;
    }

    AcquireReadLock grab(appUpgradeslock_);

    if (upgradeTableClosed_)
    {
        return;
    }

    UpgradeDescription upgradeDesc;
    if (!TryGetMessageBody(NodeInstanceIdStr, request, upgradeDesc))
    {
        return;
    }

    ApplicationIdentifier appId = upgradeDesc.ApplicationId;
    auto context = Upgrade::CancelApplicationUpgradeContext::Create(move(upgradeDesc), *this);
    upgradeMessageProcessor_->ProcessCancelUpgradeMessage(appId.ToString(), context);
}

void ReconfigurationAgent::OnFabricUpgradeStart()
{
    StateObj.OnFabricCodeUpgradeStart();
}

void ReconfigurationAgent::OnFabricUpgradeRollback()
{
    StateObj.OnFabricCodeUpgradeRollback();
}

void ReconfigurationAgent::ProcessNodeUpAck(MessageUPtr && nodeUpReply, bool isFromFMM)
{
    if (!IsOpen)
    {
        RAEventSource::Events->LifeCycleRAIgnoringNodeUpAck(NodeInstanceIdStr);
        return;
    }

    NodeUpAckMessageBody nodeUpAckBody;
    if (!TryGetMessageBody(NodeInstanceIdStr, *nodeUpReply, nodeUpAckBody))
    {
        return;
    }
    
    std::wstring activityId = wformatString(nodeUpReply->MessageId);

    RAEventSource::Events->LifeCycleNodeUpAckReceived(
        NodeInstanceIdStr, 
        isFromFMM ? FMMSource : FMSource, 
        Reliability.NodeConfig->NodeVersion,
        nodeUpAckBody, 
        activityId);

    // Process Generation Update
    GenerationHeader generationHeader = GenerationHeader::ReadFromMessage(*nodeUpReply);    
    GenerationStateManagerObj.ProcessNodeUpAck(activityId, generationHeader);

    NodeUpAckProcessorObj.ProcessNodeUpAck(move(activityId), move(nodeUpAckBody), isFromFMM);    
}

bool ReconfigurationAgent::ClientReportFaultRequestProcessor(HandlerParameters & handlerParameters, ClientReportFaultJobItemContext & context)
{
    // TODO: Refactor - this should be moved out of this file
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    if (!failoverUnit)
    {
        // This can happen in the case when the report fault job item is created first
        // but before it can be enqueued into the scheduler the FT is deleted 
        // RA should handle this and complete the context otherwise Federation will keep retrying
        CompleteReportFaultRequest(context, ErrorCode(ErrorCodeValue::REReplicaDoesNotExist, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_FOR_NON_EXISTENT_FAILOVER_UNIT)));
        return true;
    }

    // Validate
    auto hostType = Utility2::SystemServiceHelper::GetHostType(*failoverUnit);
    if (context.IsForce && hostType == Utility2::SystemServiceHelper::HostType::AdHoc)
    {
        CompleteReportFaultRequest(context, ErrorCode(ErrorCodeValue::ForceNotSupportedForReplicaOperation, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_WITH_FORCE_ON_ADHOC_SERVICE_REPLICA)));
        return true;
    }

    if (!failoverUnit->HasPersistedState && context.FaultType == FaultType::Transient)
    {
        CompleteReportFaultRequest(context, ErrorCode(ErrorCodeValue::InvalidReplicaOperation, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_TRANSIENT_FOR_NON_PERSISTED_SERVICE_REPLICA)));
        return true;
    }

    if (failoverUnit->LocalReplicaId != context.LocalReplicaId)
    {
        CompleteReportFaultRequest(context, ErrorCode(ErrorCodeValue::REReplicaDoesNotExist, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_WITH_REPLICAID_MISMATCH)));
        return true;
    }

    // Force mode is allowed if the replica is not open or is closing
    if (!context.IsForce && (!failoverUnit->LocalReplicaOpen || failoverUnit->LocalReplicaClosePending.IsSet))
    {
        CompleteReportFaultRequest(context, ErrorCode(ErrorCodeValue::InvalidReplicaStateForReplicaOperation, StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_REPORT_FAULT_WITH_INVALID_REPLICA_STATE)));
        return true;
    }

    // Handle obliterate
    if (context.IsForce)
    {
        CloseLocalReplica(handlerParameters, ReplicaCloseMode::Obliterate, context.ActivityDescription);
        CompleteReportFaultRequest(context, ErrorCodeValue::Success);
        return true;
    }

    ReportFaultHandler(handlerParameters, context.FaultType, context.ActivityDescription);

    CompleteReportFaultRequest(context, ErrorCodeValue::Success);
    return true;
}

void ReconfigurationAgent::CompleteReportFaultRequest(
    Common::ActivityId const & activityId,
    Federation::RequestReceiverContext & context, 
    Common::ErrorCode const & error)
{
    ReportFaultReplyMessageBody reply(error, error.Message);
    Transport::MessageUPtr message = RSMessage::GetReportFaultReply().CreateMessage(activityId, reply);
    federationWrapper_->CompleteRequestReceiverContext(context, move(message));
}

void ReconfigurationAgent::CompleteReportFaultRequest(ClientReportFaultJobItemContext & context, Common::ErrorCode const & error)
{
    CompleteReportFaultRequest(context.ActivityId, *context.RequestContext, error);
}

void ReconfigurationAgent::ReplicaDroppedReplyMessageProcessor(HandlerParameters & parameters, ReplicaReplyMessageContext & context)
{
    parameters.AssertFTExists();

    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaReplyMessageBody const & body = context.Body; 

    failoverUnit->ProcessReplicaDroppedReply(
        body.ErrorCode,
        body.ReplicaDescription,
        parameters.ExecutionContext);
}

void ReconfigurationAgent::ProxyUpdateServiceDescriptionReplyMessageHandler(HandlerParameters & parameters, ProxyUpdateServiceDescriptionReplyMessageContext & context)
{
    parameters.AssertFTExists();

    auto const & body = context.Body;
    auto & ft = parameters.FailoverUnit;
    auto & queue = parameters.ActionQueue;

    if (!ft->CanProcessUpdateServiceDescriptionReply(body))
    {
        return;
    }

    ft->ProcessUpdateServiceDescriptionReply(queue, body);
}

void ReconfigurationAgent::ReplicaEndpointUpdatedReplyMessageProcessor(HandlerParameters & parameters, ReplicaReplyMessageContext & context)
{
    parameters.AssertFTExists();

    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;

    failoverUnit->ProcessReplicaEndpointUpdatedReply(context.Body, parameters.ExecutionContext);
}

void ReconfigurationAgent::AddInstanceMessageProcessor(HandlerParameters & parameters, ReplicaMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaMessageBody const & msgBody = context.Body; 
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    bool canProcessResult = CheckIfAddFailoverUnitMessageCanBeProcessed(
        msgBody,
        [this, &actionQueue, &failoverUnit](ReplicaReplyMessageBody reply)
        {
            FailoverUnitTransportUtility utility(FailoverManagerId::Get(reply.FailoverUnitDescription.FailoverUnitId));
            utility.AddSendMessageToFMAction(actionQueue, RSMessage::GetAddInstanceReply(), move(reply));
        });

    if (!canProcessResult)
    {
        return;
    }

    TryCreateFailoverUnit(msgBody, failoverUnit);

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    if (failoverUnit->State == FailoverUnitStates::Open)
    {
        if (failoverUnit->LocalReplica.ReplicaId < msgBody.ReplicaDescription.ReplicaId)
        {
            CloseLocalReplica(parameters, ReplicaCloseMode::Drop, ActivityDescription::Empty);

            return;
        }
        else if (failoverUnit->LocalReplica.InstanceId == msgBody.ReplicaDescription.InstanceId)
        {
            if (failoverUnit->LocalReplica.IsReady)
            {
                ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, failoverUnit->LocalReplica.ReplicaDescription, ErrorCode::Success());

                failoverUnit->TransportUtility.AddSendMessageToFMAction(
                    actionQueue,
                    RSMessage::GetAddInstanceReply(),
                    move(body));
            }

            return;
        }
    }
    else    
    {
        if (failoverUnit->LocalReplicaInstanceId < msgBody.ReplicaDescription.InstanceId)
        {
            ASSERT_IFNOT(
                failoverUnit->LocalReplicaRole == ReplicaRole::None,
                "Incorrect FailoverUnit state: {0} msg:{1}", 
                *(failoverUnit), msgBody);

            // Create local replica
            CreateLocalReplica(parameters, msgBody, Reliability::ReplicaRole::None, ReplicaDeactivationInfo());

            ProcessReplicaOpenMessageRetry(parameters);

            return;
        }
        else if (failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId)
        {
            // FM is trying to create replica with the same instance id even though the replica is already closed
            // Need to send dropped message
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

            return;
        }
        else
        {
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

            ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.ReplicaDescription, ErrorCodeValue::StaleRequest);
            failoverUnit->TransportUtility.AddSendMessageToFMAction(
                actionQueue,
                RSMessage::GetAddInstanceReply(),
                move(body));

            return;
        }
    }
}

void ReconfigurationAgent::RemoveInstanceMessageProcessor(HandlerParameters & parameters, DeleteReplicaMessageContext & context)
{
    DeleteLocalReplica(parameters, context.Body);
}

void ReconfigurationAgent::AddPrimaryMessageProcessor(HandlerParameters & parameters, ReplicaMessageContext & context)
{   
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaMessageBody const & msgBody = context.Body; 
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    if (TryDropFMReplica(parameters, msgBody))
    {
        return;
    }

    bool canProcessResult = CheckIfAddFailoverUnitMessageCanBeProcessed(
        msgBody,
        [this, &actionQueue, &failoverUnit](ReplicaReplyMessageBody reply)
        {
            FailoverUnitTransportUtility utility(FailoverManagerId::Get(reply.FailoverUnitDescription.FailoverUnitId));
            utility.AddSendMessageToFMAction(actionQueue, RSMessage::GetAddPrimaryReply(), move(reply));
        });

    if (!canProcessResult)
    {
        return;
    }

    TryCreateFailoverUnit(msgBody, failoverUnit);

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    if (failoverUnit->State == FailoverUnitStates::Open)
    {
        if (failoverUnit->LocalReplica.ReplicaId < msgBody.ReplicaDescription.ReplicaId)
        {
            CloseLocalReplica(parameters, ReplicaCloseMode::Drop, ActivityDescription::Empty);
            return;
        }
        else if (failoverUnit->LocalReplica.InstanceId == msgBody.ReplicaDescription.InstanceId)
        {
            if (failoverUnit->LocalReplica.IsReady)
            {
                // If the service had been opened, then just send the response back
                ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, failoverUnit->LocalReplica.ReplicaDescription, ErrorCode::Success());

                failoverUnit->TransportUtility.AddSendMessageToFMAction(
                    actionQueue,
                    RSMessage::GetAddPrimaryReply(),
                    move(body));
                
                return;
            }
        }
    }
    else
    {
        if (failoverUnit->LocalReplicaInstanceId < msgBody.ReplicaDescription.InstanceId)
        {            
            ASSERT_IFNOT(
                failoverUnit->LocalReplicaRole == ReplicaRole::None,
                "Incorrect FailoverUnit state: {0} msg:{1}", 
                *(failoverUnit), msgBody);

            // Create local replica
            CreateLocalReplica(parameters, msgBody, Reliability::ReplicaRole::Primary, ReplicaDeactivationInfo(msgBody.FailoverUnitDescription.CurrentConfigurationEpoch, 0));

            ProcessReplicaOpenMessageRetry(parameters);

            return;
        }
        else if (failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId)
        {
            // FM tries to create the replica with the same instance id even though the replica is already closed
            // Need to send dropped message
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

            return;
        }
        else
        {
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

            ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.ReplicaDescription, ErrorCodeValue::StaleRequest);

            failoverUnit->TransportUtility.AddSendMessageToFMAction(
                actionQueue,
                RSMessage::GetAddPrimaryReply(),
                move(body));

            return;
        }
    }

    // Additional Staleness Checks
    if (!failoverUnit->LocalReplica.IsUp)
    {
        return;
    }

    if (!failoverUnit->LocalReplicaOpen && failoverUnit->LocalReplica.IsStandBy)
    {    
        return;
    }
    
    if (failoverUnit->LocalReplica.IsStandBy)
    {
        // Persisted replica
        ASSERT_IFNOT(
            failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId,
            "Incorrect local replica state: {0} msg:{1}", 
            *(failoverUnit), msgBody);

        CreateLocalReplica(parameters, msgBody, Reliability::ReplicaRole::Primary, ReplicaDeactivationInfo(msgBody.FailoverUnitDescription.CurrentConfigurationEpoch, 0), true /*isRebuild*/);

        ProcessReplicaOpenMessageRetry(parameters);
    }
}

void ReconfigurationAgent::AddReplicaMessageProcessor(HandlerParameters & parameters, ReplicaMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaMessageBody const & msgBody = context.Body;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;
    
    if (failoverUnit->State != FailoverUnitStates::Open || 
        !failoverUnit->LocalReplicaOpen || 
        failoverUnit->LocalReplica.IsStandBy)
    {
        // FailoverUnit has been closed or the local replica is down or in StandBy, 
        // so the Primary is no longer active, just ignore message
        return;
    }

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    ASSERT_IFNOT(failoverUnit->LocalReplicaRole == ReplicaRole::Primary,
        "AddReplica message sent to wrong replica: {0} msg:{1}", *(failoverUnit), msgBody);

    Replica* replicaToBuild = nullptr;

    Replica & replica = failoverUnit->GetReplica(msgBody.ReplicaDescription.FederationNodeId);

    if (!replica.IsInvalid)
    {
        // Replica exists
        ASSERT_IFNOT(
            replica.CurrentConfigurationRole == ReplicaRole::Idle || 
            replica.CurrentConfigurationRole == ReplicaRole::Secondary ||
            (replica.PreviousConfigurationRole == ReplicaRole::Idle && 
                replica.CurrentConfigurationRole == ReplicaRole::None),
            "Invalid replica role during AddReplica, replica:{0} msg:{1}", 
            replica, msgBody);

        // Primary is aware of the replica
        if (replica.InstanceId == msgBody.ReplicaDescription.InstanceId)
        {
            if (replica.IsReady)
            {
                // Replica has already been added, send reply message to FM
                ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, replica.ReplicaDescription, ErrorCode::Success());

                failoverUnit->TransportUtility.AddSendMessageToFMAction(
                    actionQueue,
                    RSMessage::GetAddReplicaReply(),
                    move(body));
            }
            else if (replica.IsInCreate)
            {
                // Replica is in the process of being added

                if (replica.IsUp)
                {
                    // Haven't received ack yet, resend message to remote replica
                    AddActionSendCreateReplicaMessageToRA(
                        actionQueue,
                        replica.FederationNodeInstance,
                        failoverUnit->FailoverUnitDescription,
                        replica.ReplicaDescription,
                        msgBody.ServiceDescription,
                        failoverUnit->DeactivationInfo);
                }

                if (failoverUnit->IsUpdateReplicatorConfiguration)
                {
                    // Replicator configuration still needs to be updated before this
                    // AddReplica message can be processed

                    failoverUnit->SendUpdateConfigurationMessage(actionQueue);
                }                
            }
            else if (replica.IsInDrop || replica.IsDropped)
            {
                // Stale Message
            }
            else if (replica.IsInBuild && !replica.ToBeActivated)
            {
                // Replica is in build, send message to RAP
                if (failoverUnit->IsUpdateReplicatorConfiguration)
                {
                    // Replicator configuration still needs to be updated before this
                    // AddReplica message can be processed

                    failoverUnit->SendUpdateConfigurationMessage(actionQueue);
                }
                else
                {
                    // Send message to RA-proxy so it can build the replica
                    AddActionSendMessageToRAProxy(
                        actionQueue,
                        RAMessage::GetReplicatorBuildIdleReplica(),
                        failoverUnit->ServiceTypeRegistration,
                        failoverUnit->FailoverUnitDescription, 
                        failoverUnit->LocalReplica.ReplicaDescription,
                        replica.ReplicaDescription);
                }
            }
            else if (replica.IsInBuild && replica.ToBeActivated)
            {
                failoverUnit->SendActivateMessage(replica, actionQueue);
            }
            else
            {
                ASSERT_IF(
                    !replica.IsStandBy &&
                    replica.CurrentConfigurationRole != ReplicaRole::Secondary,
                    "Invalid AddReplica {0} for {1}",
                    msgBody.ReplicaDescription, *failoverUnit);

                if (failoverUnit->IsUpdateReplicatorConfiguration)
                {
                    // Replicator configuration still needs to be updated before this
                    // AddReplica message can be processed

                    failoverUnit->SendUpdateConfigurationMessage(actionQueue);
                }
                else
                {
                    failoverUnit.EnableUpdate();

                    replica.State = ReplicaStates::InCreate;

                    replica.PackageVersionInstance = msgBody.ReplicaDescription.PackageVersionInstance;

                    replicaToBuild = &replica;
                }
            }
        }
        else if (replica.InstanceId < msgBody.ReplicaDescription.InstanceId)
        {
            // If the replica is still a secondary, we need to first remove it
            // from replication configuration.
            if (replica.CurrentConfigurationRole == ReplicaRole::Secondary)
            {
                failoverUnit.EnableUpdate();

                failoverUnit->IsUpdateReplicatorConfiguration = true;
                
                replica.UpdateInstance(msgBody.ReplicaDescription, true /*resetLSN*/);
                replica.State = ReplicaStates::InCreate;
                replica.MessageStage = ReplicaMessageStage::None; 

                replica.PackageVersionInstance = msgBody.ReplicaDescription.PackageVersionInstance;

                failoverUnit->SendUpdateConfigurationMessage(actionQueue);

                replicaToBuild = &replica;
            }
            else
            {
                // This can happen if the previous instance of the Idle replica was down
                // and FM is trying to create a new Idle replica.
                // For such cases, consider this AddReplica message as a cleanup for removing
                // the previous instance of the Idle replica.
                // Will rely on FM retry to create the replica again.
                RemoveReplica(failoverUnit, msgBody, replica, actionQueue);
            }
        }
        else if (replica.InstanceId > msgBody.ReplicaDescription.InstanceId)
        {
            // Stale Message
        }
    }
    else
    {
        // CCVersion should not change during adding new Idle replica
        ASSERT_IFNOT(
            failoverUnit->CurrentConfigurationEpoch == msgBody.FailoverUnitDescription.CurrentConfigurationEpoch,
            "Invalid current configuration version during adding Idle replica: {0} msg:{1}",
            *(failoverUnit), msgBody);

        // FailoverUnit is being updated
        failoverUnit.EnableUpdate();

        // Add replica
        Replica & idleReplica = failoverUnit->StartAddReplicaForBuild(msgBody.ReplicaDescription);

        // Set the version on the replica
        idleReplica.PackageVersionInstance = msgBody.ReplicaDescription.PackageVersionInstance;
        
        replicaToBuild = &idleReplica;
    }

    if (replicaToBuild)
    {
        AddActionSendCreateReplicaMessageToRA(
            actionQueue,
            replicaToBuild->FederationNodeInstance,
            failoverUnit->FailoverUnitDescription,
            replicaToBuild->ReplicaDescription,
            msgBody.ServiceDescription,
            failoverUnit->DeactivationInfo);
    }
}

void ReconfigurationAgent::RemoveReplicaMessageProcessor(HandlerParameters & parameters, ReplicaMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaMessageBody const & msgBody = context.Body;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    if (failoverUnit->State != FailoverUnitStates::Open || 
        !failoverUnit->LocalReplicaOpen ||
        failoverUnit->LocalReplica.IsStandBy)
    {
        // FailoverUnit has been closed or local replica is down or in StandBy, 
        // so the Primary is no longer active, just ignore message
        return;
    }

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    // Remove replica message can come to a Primary who doesn't know about the Idle replica 
    // being deleted i.e. if the existing Primary fails and a new Primary is elected. In such cases, 
    // this Primary simply acts as a pass-through for this message. It doesn't need to retry 
    // sending this message in case it doesn't receive an  ack for it since FM will eventually retry.

    // Check if this Primary knows about the Idle replica being removed

    Replica & replica = failoverUnit->GetReplica(msgBody.ReplicaDescription.FederationNodeId);

    if (replica.IsInvalid)
    {
        // Primary not aware of this replica, just send reply back
        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.ReplicaDescription, ErrorCode::Success());

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            actionQueue,
            RSMessage::GetRemoveReplicaReply(),
            move(body));
                
        return;
    }
    
    if (replica.InstanceId > msgBody.ReplicaDescription.InstanceId)
    {
        // Stale message
        return;
    }

    // FailoverUnit is being updated
    failoverUnit.EnableUpdate();

    if (replica.IsInCreate)
    {
        // Replica has not been created yet, just cleanup locally and send
        // reply to FM
        
        failoverUnit->FinishRemoveIdleReplica(replica);

        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.ReplicaDescription, ErrorCode::Success());

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            actionQueue,
            RSMessage::GetRemoveReplicaReply(),
            move(body));
        
        return;
    }

    if (!replica.IsInDrop)
    {
        // Start the process of removing the Idle replica
        failoverUnit->StartRemoveIdleReplica(replica);
    }

    ASSERT_IFNOT(replica.IsInDrop, "Replica must be in drop at this time");

    if (replica.IsUp)
    {
        // Send message to RAP so it can remove the replica
        AddActionSendMessageToRAProxy(
            actionQueue,
            RAMessage::GetReplicatorRemoveIdleReplica(),
            failoverUnit->ServiceTypeRegistration, 
            failoverUnit->FailoverUnitDescription, 
            failoverUnit->LocalReplica.ReplicaDescription,
            replica.ReplicaDescription);
    }
    else
    {
        // Replica is down
        failoverUnit->FinishRemoveIdleReplica(replica);
            
        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.ReplicaDescription, ErrorCode::Success());

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            actionQueue,
            RSMessage::GetRemoveReplicaReply(),
            move(body));
    }
}

void ReconfigurationAgent::DoReconfigurationMessageProcessor(HandlerParameters & parameters, DoReconfigurationMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    DoReconfigurationMessageBody const & msgBody = context.Body;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    /*

        Handle the case when do reconfig comes to a node which does not know about the partition
        1. Replica set is [N/P]
        2. FM sends add replica to primary. FM is now [N/P] [N/i ib]
        3. Primary gets dropped
        4. FM starts reconfig [P/S DD] [I/P IB]

        Since node never received a message it has no idea of this ft
        Add a tombstone entry and send replica dropped so fm declares data loss

    */
    TryCreateFailoverUnit(msgBody, failoverUnit);
    
    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    if (failoverUnit->IsClosed)
    {
        /*
            Replica is dropped on the node and the replica received doreconfiguration

            The node needs to send ReplicaDropped to the FM

            The replica id should be updated to the FM replica otherwise the FM will ignore replica dropped. Consider the scenario below:
            1. 0/e0 [N/P R1 n1] [N/S R2 n2] [N/S R3 n3] and min = target = 3
            2. R2 is dropped and FM decides to place R4 on n2
            3. e0/e1 [P/P R1 n1] [S/S R4 n3 IB] [S/S R3 n3]
            4. R1 is dropped and FM sends DoReconfig to R4
            5. e0/e2 [P/P R1 DD n1] [S/P R4 n3 IB] [S/S R3 n3]
            6. Since n3 has R2 as DD it will send R2 as ReplicaDropped to FM. However, FM has higher replica and it will consider it stale

            n2 should update its state to R4 DD

            If the local replica with lower instance is deleted then it must be updated to be not deleted anymore
        */
        auto localReplica = msgBody.GetReplica(NodeId);
        TESTASSERT_IF(localReplica == nullptr, "Local replica not found in DoReconfiguration {0} {1}", msgBody, *failoverUnit);
        if (localReplica == nullptr)
        {
            return;
        }

        failoverUnit->UpdateInstanceForClosedReplica(msgBody.FailoverUnitDescription, *localReplica, true, parameters.ExecutionContext);

        failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

        return;
    }

    if (!failoverUnit->LocalReplicaOpen)
    {
        // If local replica is not open, then just ignore message
        return;
    }
     
    Replica & localReplica = failoverUnit->LocalReplica;

    // Find local replica role in CC
    auto rit = 
        find_if
        (
            msgBody.BeginIterator, msgBody.EndIterator, 
            [&localReplica](Reliability::ReplicaDescription const & replicaDesc) -> bool 
            { 
                return replicaDesc.FederationNodeInstance == localReplica.FederationNodeInstance;
            }
        );
    
    ASSERT_IF(
        rit == msgBody.EndIterator,
        "Local replica not found in DoReconfiguration: {0} msg:{1}", 
        *(failoverUnit), msgBody);
    
    ReplicaDescription const & ccReplica = *(rit);
    
    if (localReplica.InstanceId > ccReplica.InstanceId)
    {
        // If the local replica's instance id is greater, then ignore this stale message
        return;
    }   

    // FailoverUnit is being updated
    // TODO: This is to be refactored still
    failoverUnit.EnableUpdate();

    // Reconfigure the replicas 
    failoverUnit->DoReconfiguration(msgBody, parameters.ExecutionContext);
}

void ReconfigurationAgent::DeleteReplicaMessageProcessor(HandlerParameters & parameters, DeleteReplicaMessageContext & context)
{
    DeleteLocalReplica(parameters, context.Body);
}

void ReconfigurationAgent::CreateReplicaMessageProcessor(HandlerParameters & parameters, RAReplicaMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    RAReplicaMessageBody const & msgBody = context.Body; 
    Federation::NodeInstance const & from = context.From;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    if (TryDropFMReplica(parameters, msgBody))
    {
        return;
    }

    bool canProcessResult = CheckIfAddFailoverUnitMessageCanBeProcessed(
        msgBody,
        [this, &actionQueue, from](ReplicaReplyMessageBody reply)
        {
            AddActionSendMessageReplyToRA(actionQueue, RSMessage::GetCreateReplicaReply(), from, move(reply));
        });

    if (!canProcessResult)
    {
        return;
    }

    TryCreateFailoverUnit(msgBody, failoverUnit);

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    if (failoverUnit->State == FailoverUnitStates::Open)
    {
        if (failoverUnit->LocalReplica.ReplicaId < msgBody.ReplicaDescription.ReplicaId)
        {
            CloseLocalReplica(parameters, ReplicaCloseMode::Drop, ActivityDescription::Empty);
            return;
        }        
    }
    else
    {
        if (failoverUnit->LocalReplicaInstanceId < msgBody.ReplicaDescription.InstanceId)
        {            
            ASSERT_IFNOT(
                failoverUnit->LocalReplicaRole == ReplicaRole::None &&
                (msgBody.ReplicaDescription.CurrentConfigurationRole == ReplicaRole::Idle ||
                 msgBody.ReplicaDescription.CurrentConfigurationRole == ReplicaRole::Secondary),
                "Incorrect FailoverUnit state: {0} msg:{1}", 
                *(failoverUnit), msgBody);

            // Create local replica
            // Set the sender ndoe after calling create local replica
            CreateLocalReplica(parameters, msgBody, ReplicaRole::Idle, msgBody.DeactivationInfo);
            failoverUnit->SenderNode = from;
            
            ProcessReplicaOpenMessageRetry(parameters);

            return;
        }
        else if (failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId)
        {
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

            return;
        }
        else
        {
            failoverUnit->SendReplicaDroppedMessage(NodeInstance, actionQueue);

            AddActionSendMessageReplyToRA(
                actionQueue,
                RSMessage::GetCreateReplicaReply(),
                context.From,
                failoverUnit->FailoverUnitDescription,
                msgBody.ReplicaDescription,
                ErrorCodeValue::StaleRequest);

            return;
        }
    }

    // Additional Staleness Checks
    if (!failoverUnit->LocalReplica.IsUp)
    {
        return;
    }

    if (!failoverUnit->LocalReplicaOpen && failoverUnit->LocalReplica.IsStandBy)
    {    
        return;
    }
    
    bool shouldRestart = false;

    if (failoverUnit->LocalReplica.IsAvailable)
    {
        // Replica needs to be closed and opened (service/replicator) if the Primary changed
        if (failoverUnit->PreviousConfigurationEpoch == Epoch::InvalidEpoch())
        {
            shouldRestart = 
                msgBody.FailoverUnitDescription.CurrentConfigurationEpoch.ToPrimaryEpoch() > 
                    failoverUnit->CurrentConfigurationEpoch.ToPrimaryEpoch();
        }
        else
        {
            shouldRestart = 
                msgBody.FailoverUnitDescription.CurrentConfigurationEpoch.ToPrimaryEpoch() > 
                    failoverUnit->PreviousConfigurationEpoch.ToPrimaryEpoch();
        }
    }

    if (shouldRestart)
    {
        // FailoverUnit is open and it receives a CreateReplica where the Primary epoch has changed, 
        // in this case, the replicator/replica needs to be closed and opened again so it can 
        // be built again (if needed) by the new Primary

        ASSERT_IFNOT(
            failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId,
            "Incorrect local replica state on Primary change: {0} msg:{1}", 
            *(failoverUnit), msgBody);

        // Restart only persisted service, close non persisted
        CloseLocalReplica(parameters, failoverUnit->HasPersistedState ? ReplicaCloseMode::Restart : ReplicaCloseMode::Drop, ActivityDescription::Empty);

        return;
    }

    if (failoverUnit->LocalReplica.IsStandBy)
    {
        // Persisted replica
        ASSERT_IFNOT(
            failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId,
            "Incorrect local replica state: {0} msg:{1}", 
            *(failoverUnit), msgBody);

        // FailoverUnit is being changed
        failoverUnit.EnableUpdate();

        // Capture CC for rebuild
        failoverUnit->CopyCCToPC();

        ReplicaRole::Enum newReplicaRole = msgBody.ReplicaDescription.CurrentConfigurationRole;

        if (!failoverUnit->LocalReplica.IsInCurrentConfiguration)
        {
            newReplicaRole = ReplicaRole::Idle;
        }
        
        // Create local replica
        CreateLocalReplica(parameters, msgBody, newReplicaRole, msgBody.DeactivationInfo, true /*isRebuild*/);
        failoverUnit->SenderNode = from;
        
        ProcessReplicaOpenMessageRetry(parameters);

        return;
    }

    if (failoverUnit->LocalReplicaInstanceId == msgBody.ReplicaDescription.InstanceId)
    {
        if (failoverUnit->LocalReplica.IsAvailable)
        {
            // Send reply to RA
            AddActionSendMessageReplyToRA(
                actionQueue,
                RSMessage::GetCreateReplicaReply(),
                from,
                failoverUnit->FailoverUnitDescription,
                failoverUnit->LocalReplica.ReplicaDescription, 
                ErrorCodeValue::Success);

            return;
        }
    }
}

void ReconfigurationAgent::GetLSNMessageProcessor(HandlerParameters & parameters, ReplicaMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaMessageBody const & msgBody = context.Body; 
    Federation::NodeInstance const & from = context.From;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    if (!failoverUnit)
    {
        /*
            The FT does not exist on the node.
            This is the edge case where the FT has been cleaned up from the node
            After the cleanup timer has expired 
            Do not waste time in performing disk IO - simply return to the primary NotFound
        */
        AddActionSendGetLsnReplicaNotFoundMessageReplyToRA(actionQueue, from, msgBody);
        return;
    }

    // If a node receives getlsn then it means that the FM is aware of the correct node instance
    // for the replica on this node which means that the replica has been uploaded
    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    if (failoverUnit->IsClosed)
    {
        TESTASSERT_IF(failoverUnit->LocalReplicaId == 0, "FT cannot be uninitialized here {0}", *failoverUnit);

        // Update CC Epoch for staleness checks
        failoverUnit.EnableUpdate();
        failoverUnit->CurrentConfigurationEpoch = msgBody.FailoverUnitDescription.CurrentConfigurationEpoch;

        /*
            There are two main cases to consider here:
            - GetLSN comes for the same replica which is on the node
            - GetLSN comes for a different replica. Example: 
                (a) replica r1 is dropped on this node
                (b) FM starts reconfiguration S/S IB to start creating a replica on this node with r2
                (c) Before CreateReplica can be sent the primary goes down
                (d) the subsequent reconfiguration, GetLSN will be sent to this node with S/S r2 IB. This node has Closed, r1
                    In this case, it should reply with NotFound instead of marking S/S r2 DD as r2 was never created

        */
        if (msgBody.ReplicaDescription.ReplicaId == failoverUnit->LocalReplicaId)
        {
            // FailoverUnit not in expected state, just return reply            
            // Versions of RA prior to 4.0 will assert if GetLSNReply contains a higher instance than what the primary knows
            auto instanceId = msgBody.ReplicaDescription.InstanceId;
            ReplicaDescription localReplicaDesc(
                NodeInstance,
                failoverUnit->LocalReplicaId,
                instanceId,
                ReplicaRole::None,
                ReplicaRole::None,
                Reliability::ReplicaStates::Dropped);

            AddActionSendMessageReplyToRA(
                actionQueue,
                RSMessage::GetGetLSNReply(),
                from,
                failoverUnit->FailoverUnitDescription,
                localReplicaDesc,
                ErrorCodeValue::Success);
        }
        else
        {
            AddActionSendGetLsnReplicaNotFoundMessageReplyToRA(actionQueue, from, msgBody);
        }        
    }
    else if (failoverUnit->LocalReplicaOpen && !failoverUnit->LocalReplicaClosePending.IsSet)
    {    
        failoverUnit->ProcessGetLSNMessage(from, msgBody, parameters.ExecutionContext);
    }
}

void ReconfigurationAgent::DeactivateMessageProcessor(HandlerParameters & parameters, DeactivateMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    DeactivateMessageBody const & msgBody = context.Body; 
    Federation::NodeInstance const & from = context.From;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    // If a node receives deactivate then it means that the FM is aware of the correct node instance
    // for the replica on this node which means that the replica has been uploaded
    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    // Check if the FailoverUnit has already been closed
    if (failoverUnit->IsClosed)
    {
        // FailoverUnit is being updated
        failoverUnit.EnableUpdate();

        // FailoverUnit is closed, just update the CCVersion
        failoverUnit->CurrentConfigurationEpoch = msgBody.FailoverUnitDescription.CurrentConfigurationEpoch;

        // By the time DeactivateReply is sent localReplica instance id could be different than
        // the incoming instance id due to restarts. If the FailoverUnit is closed, it is ok to send 
        // with the instanceid in the message

        auto locaReplicaId = failoverUnit->LocalReplicaId;

        // Find local replica instance id in CC
        auto rit = 
            find_if
            (
                msgBody.BeginIterator, msgBody.EndIterator, 
                [&locaReplicaId](Reliability::ReplicaDescription const & replicaDesc) -> bool 
                { 
                    return replicaDesc.ReplicaId == locaReplicaId;
                }
            );

        if (rit != msgBody.EndIterator)
        {
            ReplicaDescription const & ccReplica = *(rit);

            AddActionSendMessageReplyToRA(
                actionQueue,
                RSMessage::GetDeactivateReply(),
                from,
                failoverUnit->FailoverUnitDescription,
                ReplicaDescription(NodeInstance, ccReplica.ReplicaId, ccReplica.InstanceId),
                ErrorCodeValue::Success);
        }
        
        return;
    }
    else
    {    
        Replica & localReplica = failoverUnit->LocalReplica;

        if (!failoverUnit->LocalReplicaOpen || localReplica.IsInCreate)
        {
            // Replica not ready to process any messages yet, ignore
            return;
        }

        // Find local replica role in CC
        auto rit = 
            find_if
            (
                msgBody.BeginIterator, msgBody.EndIterator, 
                [&localReplica](Reliability::ReplicaDescription const & replicaDesc) -> bool 
                { 
                    return replicaDesc.FederationNodeInstance == localReplica.FederationNodeInstance;
                }
            );

        ASSERT_IF(
            rit == msgBody.EndIterator,
            "Local replica not found in Deactivate message: {0} msg:{1}", 
            *(failoverUnit), msgBody);

        ReplicaDescription const & ccReplica = *(rit);

        if (localReplica.InstanceId > ccReplica.InstanceId)
        {
            // If the local replica's instance id is greater, then ignore this stale message
            return;
        }

        if (msgBody.IsForce)
        {
            // Handle the restart messages here for simplicity
            // These messages must not call start deactivate because that expects a valid deactivation epoch
            // The replica id, instance id, cc epoch checks are sufficient to detect staleness

            // It is important to process Phase1_Deactivate and Phase2_Deactivate here
            // Phase3_Deactivate expects the progress vector to be transmitted and StartDeactivate validates this
            ReplicaCloseMode closeMode = failoverUnit->HasPersistedState ? ReplicaCloseMode::Restart : ReplicaCloseMode::Deactivate;
            Federation::NodeInstance senderNode = failoverUnit->HasPersistedState ? ReconfigurationAgent::InvalidNode : from;

            CloseLocalReplica(parameters, closeMode, from, ActivityDescription::Empty);

            return;
        }

        // If PC epoch is empty and CC epoch matches, then the replica must have received the Activate message,
        // and hence this is a stale Deactivate message 
        bool isStale = 
            failoverUnit->PreviousConfigurationEpoch == Epoch::InvalidEpoch() &&
            failoverUnit->CurrentConfigurationEpoch == msgBody.FailoverUnitDescription.CurrentConfigurationEpoch &&
            localReplica.IsInConfiguration;

        if (!isStale)
        {
            isStale = failoverUnit->IsConfigurationMessageBodyStale(msgBody);
        }      

        if (isStale)
        {
            return;
        }

        /*
            The deactivate message requires the following processing to happen. Each of these is independent.
            The isStale boolean decides staleness for the message. 
            
            Deactivate of S/N must drop the replica. The DeactivateReply will be sent after the replica closes
            
            Deactivate message requires the configuration to be updated for rebuild. This requires processing:
            - All Deactivate messages for SB replicas 
            - The very first deactivate message received by a replica that is N/S. 
            - A deactivate message received by S/I replica
            - A deactivate message received by a replica where ccEpoch > icEpoch. Deactivate processing will update
              the icEpoch so that deactivate will continue to happen
            - Subsequent deactivate messages with the same ccEpoch should not cause the configuration to be updated
            - Deactivate message received by a replica where the pcRole in the incoming message body is Idle does not require state update. 
              This can happen as part of a reconfiguration where [P/P] [I/S] and the I/S replica restarts prior to activate. 
              At this time the primary will build it again and send it deactivate to update the deactivation info. This deactivate
              is being received while this replica is not part of PC and does not need to be processed

            Deactivate message needs to update the deactivation epoch
            - This can happen if the replica completes build in Phase2_Catchup. The deactivation epoch will be the current
              deactivation epoch of the primary. There will be another deactivate message sent when the primary enters Phase3 
              Deactivate after catchup completes at which point the deactivation epoch becomes the CC Epoch
            - This can happen if the replica completes build in Phase3 Deactivate. The deactivation epoch will be the current
              deactivation epoch of the primary
        */
        
        {
            // Perform this before updating the local replica state via StartDeactivate
            failoverUnit->UpdateDeactivationInfo(msgBody.FailoverUnitDescription, ccReplica, msgBody.DeactivationInfo, parameters.ExecutionContext);
        }

        bool updateState =
                (localReplica.IsStandBy ||
                localReplica.PreviousConfigurationRole == ReplicaRole::None ||
                failoverUnit->IntermediateConfigurationEpoch < msgBody.FailoverUnitDescription.CurrentConfigurationEpoch) &&
                ccReplica.PreviousConfigurationRole > ReplicaRole::Idle;

        if (updateState)
        {
            failoverUnit->StartDeactivate(msgBody, parameters.ExecutionContext);
        }

        if (ccReplica.CurrentConfigurationRole == ReplicaRole::None)
        {
            // I/S->N transition
            // This replica representing the FailoverUnit on this node is no longer participating in CC, 
            // need to clean itself up
            // The return is needed to ensure that deactivate reply is not sent
            CloseLocalReplica(parameters, ReplicaCloseMode::Deactivate, from, ActivityDescription::Empty);
            return;
        }

        if (updateState)
        {
            failoverUnit->FinishDeactivate(ccReplica);
        }

        AddActionSendMessageReplyToRA(
            actionQueue,
            RSMessage::GetDeactivateReply(),
            from,
            failoverUnit->FailoverUnitDescription,
            ReplicaDescription(NodeInstance, failoverUnit->LocalReplicaId, failoverUnit->LocalReplicaInstanceId),
            ErrorCodeValue::Success);
    }
}

void ReconfigurationAgent::ActivateMessageProcessor(HandlerParameters & parameters, ActivateMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    auto const & msgBody = context.Body;
    Federation::NodeInstance const & from = context.From;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    TryUpdateServiceDescription(failoverUnit, actionQueue, msgBody.ServiceDescription);

    // If a node receives deactivate then it means that the FM is aware of the correct node instance
    // for the replica on this node which means that the replica has been uploaded
    failoverUnit->OnReplicaUploaded(parameters.ExecutionContext);

    if (failoverUnit->State != FailoverUnitStates::Open)
    {
        failoverUnit.EnableUpdate();

        // FailoverUnit is closed, just update the CCVersion
        failoverUnit->CurrentConfigurationEpoch = msgBody.FailoverUnitDescription.CurrentConfigurationEpoch;

        // By the time ActivateReply is sent localReplica instance id could be different than
        // the incoming instance id due to restarts. If the FailoverUnit is closed, it is ok to send 
        // with the instanceid in the message

        auto locaReplicaId = failoverUnit->LocalReplicaId;

        // Find local replica instance id in CC
        auto rit = 
            find_if
            (
                msgBody.BeginIterator, msgBody.EndIterator, 
                [&locaReplicaId](Reliability::ReplicaDescription const & replicaDesc) -> bool 
                { 
                    return replicaDesc.ReplicaId == locaReplicaId;
                }
            );

        if (rit != msgBody.EndIterator)
        {
            ReplicaDescription const & ccReplica = *(rit);

            AddActionSendMessageReplyToRA(
                actionQueue,
                RSMessage::GetActivateReply(),
                from,
                failoverUnit->FailoverUnitDescription,
                ReplicaDescription(NodeInstance, ccReplica.ReplicaId, ccReplica.InstanceId),
                ErrorCodeValue::Success);
        }
        
        return;
    }
    else
    {    
        Replica & localReplica = failoverUnit->LocalReplica;

        if (!failoverUnit->LocalReplicaOpen || failoverUnit->LocalReplicaClosePending.IsSet)
        {
            // Replica not ready to process any messages yet, ignore
            return;
        }

        // Find local replica role in CC
        auto rit = 
            find_if
            (
                msgBody.BeginIterator, msgBody.EndIterator, 
                [&localReplica](Reliability::ReplicaDescription const & replicaDesc) -> bool 
                { 
                    return replicaDesc.FederationNodeInstance == localReplica.FederationNodeInstance;
                }
            );
        
        ASSERT_IF(
            rit == msgBody.EndIterator,
            "Local replica not found in Activate: {0} message:{1}", 
            *(failoverUnit), msgBody);

        ASSERT_IF(rit->CurrentConfigurationRole == ReplicaRole::Primary, "Cannot have activate message sent to a primary {0} {1}", msgBody, *failoverUnit);
        
        ReplicaDescription const & ccReplica = *(rit);
        
        if (localReplica.InstanceId > ccReplica.InstanceId)
        {
            // If the local replica's instance id is greater, then ignore this stale message
            return;
        }
        
        // Always update the deactivation info on receipt of an activate message
        // At this time it is verified that this message cannot be stale from the 
        // deactivation info perspective because the CC epoch matches (it cannot be greater)
        // the UpdateDeactivationInfo method will ensure that it updates only if there is a change
        failoverUnit->UpdateDeactivationInfo(msgBody.FailoverUnitDescription, ccReplica, msgBody.DeactivationInfo, parameters.ExecutionContext);

        if (failoverUnit->LocalReplica.MessageStage == ReplicaMessageStage::RAProxyReplyPending)
        {
            TESTASSERT_IF(!failoverUnit->LocalReplica.IsInCurrentConfiguration, "ProxyReplyPending during activate {0}\r\n{1}", *failoverUnit, msgBody);
            
            // Previous activate (I/S) is in progress
            // Use this as a retry
            // There is no need to update the configuration because the replica set could not have changed
            // This is because I/S was in progress and never finished and any subsequent failover would still keep the same replica set
            // as the FM cannot change the replica set during reconfiguration
            //
            // The deactivation info can be updated which is handled above

            // Update the sender node because the message may be coming from a different replica
            failoverUnit->SenderNode = context.From;

            // Message to RAP is pending, resend
            failoverUnit->SendUpdateConfigurationMessage(actionQueue);

            return;
        }

        if ((msgBody.FailoverUnitDescription.CurrentConfigurationEpoch == failoverUnit->CurrentConfigurationEpoch) &&
            (msgBody.FailoverUnitDescription.PreviousConfigurationEpoch < failoverUnit->PreviousConfigurationEpoch) &&
            (ccReplica.IsStandBy && !localReplica.IsStandBy))
        {
            // Stale message
            return;
        }

        if (failoverUnit->IsConfigurationMessageBodyStale(msgBody))
        {
            return;
        }

        if (localReplica.IsStandBy ||
             (msgBody.FailoverUnitDescription.CurrentConfigurationEpoch > 
                failoverUnit->CurrentConfigurationEpoch) || 
            (ccReplica.CurrentConfigurationRole >= ReplicaRole::Secondary &&
                (localReplica.CurrentConfigurationRole != ccReplica.CurrentConfigurationRole ||
                localReplica.PreviousConfigurationRole != ReplicaRole::None)))
        {
            ReplicaRole::Enum preLocalReplicaRole = failoverUnit->LocalReplica.CurrentConfigurationRole;

            TESTASSERT_IF(failoverUnit->LocalReplica.MessageStage == ReplicaMessageStage::RAProxyReplyPending, "Cannot have RAP Pending here {0}", *failoverUnit);
            
            failoverUnit->StartActivate(msgBody, parameters.ExecutionContext);

            if (!ccReplica.IsStandBy && preLocalReplicaRole != ccReplica.CurrentConfigurationRole)
            {
                failoverUnit->LocalReplica.MessageStage = ReplicaMessageStage::RAProxyReplyPending;
                failoverUnit->SenderNode = from;

                failoverUnit->SendUpdateConfigurationMessage(actionQueue);

                return;
            }
                
            // Local replica is not undergoing change, just finish Activate
            failoverUnit->FinishActivate(
                failoverUnit->LocalReplica.ReplicaDescription, 
                false /*updateEnpoints*/,
                parameters.ExecutionContext);
        }
    }

    AddActionSendMessageReplyToRA(
        actionQueue,
        RSMessage::GetActivateReply(),
        from,
        failoverUnit->FailoverUnitDescription,
        failoverUnit->LocalReplica.ReplicaDescription,
        ErrorCodeValue::Success);
}

void ReconfigurationAgent::CreateReplicaReplyMessageProcessor(HandlerParameters & parameters, ReplicaReplyMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaReplyMessageBody const & msgBody = context.Body;
    StateMachineActionQueue & actionQueue = parameters.ActionQueue;

    ErrorCodeValue::Enum errorCodeValue = msgBody.ErrorCode.ReadValue();

    if (failoverUnit->State != FailoverUnitStates::Open ||
        (failoverUnit->LocalReplicaRole != ReplicaRole::Primary &&
            !failoverUnit->AllowBuildDuringSwapPrimary) ||
        !failoverUnit->LocalReplicaOpen)
    {
        // FailoverUnit has undergone reconfiguration before the reply was received,
        // just ignore message

        return;
    }
    
    // Get the replica
    Replica & replica = failoverUnit->GetReplica(msgBody.ReplicaDescription.FederationNodeId);

    if (replica.IsInvalid || 
        replica.InstanceId != msgBody.ReplicaDescription.InstanceId ||
        !replica.IsUp)
    {
        // Replica state has changed, ignore stale message

        return;
    }

    // Replica in the process of being created
    if (replica.IsInCreate)
    {
        // FailoverUnit is being updated
        failoverUnit.EnableUpdate();

        if (errorCodeValue == ErrorCodeValue::Success)
        {
            // Finish creating the Idle replica
            failoverUnit->FinishAddIdleReplica(replica, msgBody.ReplicaDescription, parameters.ExecutionContext);
        }
        else
        {
            // Send AddReplica failure reply to FM only for Idle replicas
            if (replica.CurrentConfigurationRole == ReplicaRole::Idle)
            {
                // Abort creation of Idle replica
                failoverUnit->AbortAddIdleReplica(replica);

                ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.ReplicaDescription, errorCodeValue);;

                failoverUnit->TransportUtility.AddSendMessageToFMAction(
                    actionQueue,
                    RSMessage::GetAddReplicaReply(),
                    move(body));
             }

            return;
        }
    }

    // Replica in the process of being built
    if (replica.IsInBuild)
    {
        // FailoverUnit is being updated
        failoverUnit.EnableUpdate();

        // Send message to RA-proxy so it can build the replica
        AddActionSendMessageToRAProxy(
            actionQueue,
            RAMessage::GetReplicatorBuildIdleReplica(),
            failoverUnit->ServiceTypeRegistration, 
            failoverUnit->FailoverUnitDescription, 
            failoverUnit->LocalReplica.ReplicaDescription,
            replica.ReplicaDescription);
    }
    else if (replica.IsReady && !failoverUnit->IsReconfiguring)
    {
        // Replica created and built, sent reply to FM
        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, replica.ReplicaDescription, ErrorCode::Success());

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            actionQueue,
            RSMessage::GetAddReplicaReply(),
            move(body));
    }
}

void ReconfigurationAgent::GetLSNReplyMessageProcessor(HandlerParameters & parameters, GetLSNReplyMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    GetLSNReplyMessageBody const & msgBody = context.Body;

    ErrorCodeValue::Enum errCodeValue = msgBody.ErrorCode.ReadValue();

    failoverUnit->ProcessGetLSNReply(
        msgBody.FailoverUnitDescription,
        msgBody.ReplicaDescription,
        msgBody.DeactivationInfo,
        errCodeValue,
        parameters.ExecutionContext);
}

void ReconfigurationAgent::DeactivateReplyMessageProcessor(HandlerParameters & parameters, ReplicaReplyMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaReplyMessageBody const & msgBody = context.Body;

    ErrorCodeValue::Enum errCodeValue = msgBody.ErrorCode.ReadValue();

    if (!failoverUnit->LocalReplicaOpen || failoverUnit->LocalReplicaClosePending.IsSet)
    {
        // FailoverUnit is closed or local replica is down before the completed message was received
        // Note: we cannot check that the partition role is Primary, due to swap primary scenario

        return;
    }

    if (failoverUnit->CurrentConfigurationEpoch < msgBody.FailoverUnitDescription.CurrentConfigurationEpoch)
    {
        Assert::TestAssert(
            "Unexpected configuration version received during DeactivateReply: {0} msg:{1}",
            *(failoverUnit), msgBody);
        return;
    }

    if (errCodeValue == ErrorCodeValue::Success)
    {
        // Deactivation completed
        failoverUnit->DeactivateCompleted(msgBody.ReplicaDescription, parameters.ExecutionContext);
    }
}

void ReconfigurationAgent::ActivateReplyMessageProcessor(HandlerParameters & parameters, ReplicaReplyMessageContext & context)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ReplicaReplyMessageBody const & msgBody = context.Body;
    ErrorCodeValue::Enum errCodeValue = msgBody.ErrorCode.ReadValue();

    if (failoverUnit->State != FailoverUnitStates::Open || !failoverUnit->LocalReplicaOpen)
    {
        // FailoverUnit is closed or local replica is down before the completed message was received
        // Note: we cannot check that the partition role is Primary, due to swap primary scenario

        return;
    }

    if (failoverUnit->CurrentConfigurationEpoch < msgBody.FailoverUnitDescription.CurrentConfigurationEpoch)
    {
        // Assert in test - ignore input in prod
        Assert::TestAssert(
            "Unexpected configuration version received during ActivateReply: {0} msg:{1}",
            *(failoverUnit), msgBody);
        return;
    }

    if (errCodeValue == ErrorCodeValue::Success)
    {
        failoverUnit->ActivateCompleted(msgBody.ReplicaDescription, parameters.ExecutionContext);
    }
}

ServicePackageVersionInstance ReconfigurationAgent::GetServicePackageVersionInstance(
    ServicePackageVersionInstance const & ftVersionInstance,
    bool isUp,
    bool hasRegistration,
    ServicePackageVersionInstance const & messageVersion,
    ServicePackageVersionInstance const & upgradeTableVersion)
{
    TESTASSERT_IF(ftVersionInstance == ServicePackageVersionInstance::Invalid && (isUp || hasRegistration), "FT must be open to be either up or have service package registration and hence the version instance cannot be invalid");

    /*

    1. Determine the highest known version instance for this service package from message and upgrade table. This is the desired version.
    2. If the desired version is less than or equal to the version on the FT then no-change
    3. If the desired version is greater than the version instance on the FT:
        a. If the FT is Down then it is safe to update the version instance at this point
        b. If the FT Is Up but does not have service type registration then it is safe to update the version at this point
        c. If the FT is Up then either an upgrade is being processed OR an upgrade will arrive and the upgrade will update the version instance */
    
    ServicePackageVersionInstance desiredVersionInstance;
    if (upgradeTableVersion == ServicePackageVersionInstance::Invalid)
    {
        desiredVersionInstance = messageVersion;
    }
    else 
    {
        desiredVersionInstance = messageVersion.InstanceId > upgradeTableVersion.InstanceId ? messageVersion : upgradeTableVersion;    
    }

    if (ftVersionInstance == ServicePackageVersionInstance::Invalid)
    {
        return desiredVersionInstance;
    }

    if (ftVersionInstance.InstanceId >= desiredVersionInstance.InstanceId)
    {
        return ftVersionInstance;
    }
    
    if (!isUp)
    {
        return desiredVersionInstance;
    }

    return hasRegistration ? ftVersionInstance : desiredVersionInstance;
}

ServicePackageVersionInstance ReconfigurationAgent::GetServicePackageVersionInstance(
    LockedFailoverUnitPtr const & failoverUnit,
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageVersionInstance const & msgVersionInstance)
{
    ServiceModel::ServicePackageVersionInstance upgradeTableVersionInstance = ServicePackageVersionInstance::Invalid;
    ServiceModel::ServicePackageVersionInstance ftVersionInstance = ServicePackageVersionInstance::Invalid;

    if (failoverUnit->IsOpen)
    {
        ftVersionInstance = failoverUnit->LocalReplica.PackageVersionInstance;
    }

    // The appUpgradesLock is important here
    // The race over here is between a new FT being created and the upgrade message processing enumerating the LFUM for the list of FTs
    // Thus, if a FT is being created/added to the LFUM after the upgrade message processing has enumerated the LFUM for the list of FTs
    // It must see the latest version for that service package identifier
    // This is achieved by performing the update of the upgradeVersions and enumeration of the LFUM under this lock in the NodeUpgradeHandler
    // At the point of time when this code is being processed this FT is already in the LFUM and the GetAllFailoverUnitIds method on the LFUM will return this
    {
        AcquireReadLock grab(appUpgradeslock_);

        auto it = upgradeVersions_.find(servicePackageId);
        if (it != upgradeVersions_.end())        
        {
            upgradeTableVersionInstance = it->second;
        }
    }

    return GetServicePackageVersionInstance(
        ftVersionInstance,
        failoverUnit->IsOpen ? failoverUnit->LocalReplica.IsUp : false,
        failoverUnit->ServiceTypeRegistration != nullptr,
        msgVersionInstance,
        upgradeTableVersionInstance);
}

// Remove given replica
void ReconfigurationAgent::RemoveReplica(
        LockedFailoverUnitPtr & failoverUnit, 
        ReplicaMessageBody const & ,
        Replica & replica,
        StateMachineActionQueue & actionQueue)
{
    // FailoverUnit is being changed
    failoverUnit.EnableUpdate();

    if (replica.IsInCreate)
    {
        // Replica has not been created yet, just cleanup locally and continue
        // adding the new replica
        failoverUnit->FinishRemoveIdleReplica(replica);
    }
    else 
    {
        // Replica is in build or has been created, need to cleanup RAP/replicator
        // state for this replica
        if (!replica.IsInDrop)
        {
            // Start the process of removing the Idle replica
            failoverUnit->StartRemoveIdleReplica(replica);
        }

        if (replica.IsInDrop)
        {
            // Send message to RA-proxy so it can remove the replica
            AddActionSendMessageToRAProxy(
                actionQueue,
                RAMessage::GetReplicatorRemoveIdleReplica(),
                failoverUnit->ServiceTypeRegistration,
                failoverUnit->FailoverUnitDescription, 
                failoverUnit->LocalReplica.ReplicaDescription,
                replica.ReplicaDescription);
        }
    }
}

bool ReconfigurationAgent::FailoverUnitCleanupProcessor(
    HandlerParameters & handlerParameters, 
    JobItemContextBase &)
{
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;
    StateMachineActionQueue & queue = handlerParameters.ActionQueue;

    // This job item cannot be processed
    if (!failoverUnit->CleanupPendingFlag.IsSet)
    {
        return false;
    }

    TESTASSERT_IF(!failoverUnit->LocalReplicaDeleted || failoverUnit->IsOpen, "FT must be deleted and closed for cleanup to run {0}", *failoverUnit);
    if (!failoverUnit->LocalReplicaDeleted || failoverUnit->IsOpen)
    {
        return false;
    }

    TimeSpan elapsedTimeSinceClose = clock_->DateTimeNow() - failoverUnit->LastUpdatedTime;

    if (elapsedTimeSinceClose <= Config.DeletedFailoverUnitTombstoneDuration)
    {
        // This is a deleted FT
        // for which the tombstone duration has not yet been reached
        // Keep this in the set and verify again at the next iteration
        return false;
    }
    else
    {
        // Delete this FT
        failoverUnit->CleanupPendingFlag.SetValue(false, queue);
        failoverUnit.MarkForDelete();
        return true;
    }
}

bool ReconfigurationAgent::ReconfigurationMessageRetryProcessor(HandlerParameters & handlerParameters, JobItemContextBase &)
{
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    if (!failoverUnit->MessageRetryActiveFlag.IsSet)
    {
        return false;
    }

    failoverUnit->ProcessMsgResends(handlerParameters.ExecutionContext);

    return true;
}

bool ReconfigurationAgent::NodeUpdateServiceDescriptionProcessor(
    NodeUpdateServiceRequestMessageBody const & body, 
    HandlerParameters & handlerParameters, 
    NodeUpdateServiceJobItemContext & context)
{
    ASSERT_IF(handlerParameters.Work == nullptr, "Work must be set");
    ASSERT_IF(context.WasUpdated, "Context must not have been updated by now");

    ServiceDescription const & updatedServiceDescription = body.ServiceDescription;
    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;

    bool updated = TryUpdateServiceDescription(failoverUnit, handlerParameters.ActionQueue, updatedServiceDescription);
    
    if (updated)
    {
        context.MarkUpdated();
    }

    return updated;
}

void ReconfigurationAgent::OnNodeUpdateServiceDescriptionProcessingComplete(MultipleEntityWork & work, NodeUpdateServiceRequestMessageBody const & body)
{
    // We need to send a reply to FM 
    // The only case in which we suppress the reply is if the commit for any job item
    // for which an update was issued fails -> then we need to wait for FM retry
    bool suppressReply = any_of(work.JobItems.begin(), work.JobItems.end(), [] (EntityJobItemBaseSPtr const & ji)
    {        
        auto const & context = ji->GetContext<NodeUpdateServiceJobItem>();
        return context.WasUpdated && !ji->CommitResult.IsSuccess(); 
    });

    if (!suppressReply)
    {
        // Send a reply to FM
        NodeUpdateServiceReplyMessageBody reply(
            body.ServiceDescription.Name,
            body.ServiceDescription.Instance,
            body.ServiceDescription.UpdateVersion);

        FMTransportObj.SendMessageToFM(body.Owner, RSMessage::GetNodeUpdateServiceReply(), work.ActivityId, reply);
    }
}

void ReconfigurationAgent::CreateReplicaUpReplyJobItemAndAddToList(
    Reliability::FailoverUnitInfo && ftInfo,
    bool isInDroppedList,
    MultipleEntityWorkSPtr const & work,
    RAJobQueueManager::EntryJobItemList & jobItemList,
    vector<FailoverUnitId> & acknowledgedFTs)
{
    acknowledgedFTs.push_back(ftInfo.FailoverUnitId);

    auto entry = LocalFailoverUnitMapObj.GetEntry(ftInfo.FailoverUnitId);
    if (entry == nullptr)
    {
        return;
    }

    auto handler = [](HandlerParameters & handlerParameters, ReplicaUpReplyJobItemContext & context)
    {
        return handlerParameters.RA.ReplicaUpReplyProcessor(handlerParameters, context);
    };

    ReplicaUpReplyJobItemContext context(move(ftInfo), isInDroppedList);
    ReplicaUpReplyJobItem::Parameters jobItemParameters(
        entry,
        work,
        handler,
        static_cast<JobItemCheck::Enum>(JobItemCheck::RAIsOpenOrClosing | JobItemCheck::FTIsNotNull),
        *JobItemDescription::ReplicaUpReply,
        move(context));

    auto jobItem = make_shared<ReplicaUpReplyJobItem>(move(jobItemParameters));

    auto element = RAJobQueueManager::EntryJobItemListElement(move(entry), move(jobItem));

    jobItemList.push_back(move(element));
}

void ReconfigurationAgent::ProcessReplicaUpReplyList(
    vector<FailoverUnitInfo> & ftInfos,
    bool isDroppedList,
    MultipleEntityWorkSPtr const & work,
    RAJobQueueManager::EntryJobItemList & jobItemList,
    vector<Reliability::FailoverUnitId> & acknowledgedFTs)
{
    for (size_t i = 0; i < ftInfos.size(); i++)
    {
        FailoverUnitInfo & fuInfo = ftInfos[i];
        CreateReplicaUpReplyJobItemAndAddToList(
            move(fuInfo),
            isDroppedList,
            work,
            jobItemList,
            acknowledgedFTs);
    }
}

void ReconfigurationAgent::ReplicaUpReplyMessageHandler(Message & request)
{
    ReplicaUpMessageBody body;
    if (!TryGetMessageBody(NodeInstanceIdStr, request, body))
    {
        return;
    }
    
    vector<FailoverUnitId> acknowledgedFTs;
    RAJobQueueManager::EntryJobItemList jobItemList;

    // It is important to tell the pending replica upload state about the 
    // islast flag in the message only after all the fts have been processed
    // Otherwise the pending replica upload state will assert that it has 
    // some fts still pending and the isLast has been acknowledged
    auto isLast = body.IsLastReplicaUpMessage;
    auto senderFM = body.Sender;
    auto activityId = wformatString(request.MessageId);
    MultipleEntityWorkSPtr work = make_shared<MultipleEntityWork>(
        activityId,
        [this, isLast, senderFM, activityId](MultipleEntityWork & inner, ReconfigurationAgent &)
        {
            // It is important to check all the following conditions
            // If the RA is closed at this point of time then the last replica up reply may
            // not have been fully processed and hence the pending upload state cannot be updated
            if (isLast && IsOpen && inner.GetFirstError().IsSuccess())
            {
                nodeState_->GetPendingReplicaUploadStateProcessor(senderFM).ProcessLastReplicaUpReply(activityId);
            }
        });
    
    ProcessReplicaUpReplyList(body.GetReplicaList(), false, work, jobItemList, acknowledgedFTs);

    ProcessReplicaUpReplyList(body.GetDroppedReplicas(), true, work, jobItemList, acknowledgedFTs);

    JobQueueManager.AddJobItemsAndStartMultipleFailoverUnitWork(work, jobItemList);
}

bool ReconfigurationAgent::ReplicaUpReplyProcessor(HandlerParameters & handlerParameters, ReplicaUpReplyJobItemContext & context)
{
    handlerParameters.AssertFTExists();

    LockedFailoverUnitPtr & failoverUnit = handlerParameters.FailoverUnit;
    StateMachineActionQueue & queue = handlerParameters.ActionQueue;

    // Staleness checks
    if (failoverUnit->LocalReplicaId != context.FTInfo.LocalReplica.ReplicaId)
    {
        return true;
    }

    // Service Description Updates
    TryUpdateServiceDescription(failoverUnit, queue, context.FTInfo.ServiceDescription);

    // Tell the FT about replica up reply
    failoverUnit->ProcessReplicaUpReply(context.FTInfo.LocalReplica, handlerParameters.ExecutionContext);

    // Now process the actual request by the FM
    if (context.IsInDroppedList)
    {
        // The following scenarios are important here:

        // The FM has sent this in the dropped list i.e. the replica is deleted on the FM
        // The FT on the RA is Closed -> simply perform the delete
        if (failoverUnit->IsClosed)
        {
            CloseLocalReplica(handlerParameters, ReplicaCloseMode::ForceDelete, ActivityDescription::Empty);
            return true;
        }

        //
        // Replica is still open pending and the runtime is not active
        //   In this case it is possible that the runtime will never come back (upgrades where service types are removed etc)
        //   The replica must be aborted and state leaked (this behavior is old v1 behavior)
        //   TODO: See if we can improve this - it effectively implies there is a ReopenSuccessWaitInterval timeout
        //   for deleted applications to register their service types on node restart
        if (failoverUnit->LocalReplicaOpenPending.IsSet && !failoverUnit->IsRuntimeActive)
        {
            CloseLocalReplica(handlerParameters, ReplicaCloseMode::ForceDelete, ActivityDescription::Empty);
            return true;
        }

        //
        // The RA is reopening this replica (it has the registration) or the node is deactivated and FM has asked for the drop
        if (nodeState_->GetNodeDeactivationState(failoverUnit->Owner).IsActivated)
        {
            // This could be in a scenario where the open is taking a long time and RA sent SB U i2 as SB D i1
            // Or a scenario where the open has completed and the RA sent SB U i2 and got back DD D i2
            // Start the drop over here since RA now supports transition from reopening to dropping
            // The drop must only happen if the node is activated
            CloseLocalReplica(handlerParameters, ReplicaCloseMode::ForceDelete, ActivityDescription::Empty);
            return true;
        }
        else 
        {
            // Node is deactivated so the replica could be either up or down
            // If the replica is down delete it using QueuedDelete so that we gracefully call CR(None) when 
            // the node gets activated
            // If the replica is up and the node is deactivated then the replica is most likely already closing
            // Try to close it again with ForceDelete which will do the right thing
            if (failoverUnit->LocalReplica.IsUp)
            {
                CloseLocalReplica(handlerParameters, ReplicaCloseMode::ForceDelete, ActivityDescription::Empty);
                return true;
            }
            else
            {
                CloseLocalReplica(handlerParameters, ReplicaCloseMode::QueuedDelete, ActivityDescription::Empty);
                return true;
            }
        }
    }
    else
    {
        // For the same replica id and instance id the FM cannot mark the replica as DD in the Processed List if RA does not have it as closed
        // This is because the RA is the authority here and if RA did not send the replica as Dropped and FM wants to drop it
        // The replica should be put in the Dropped List
        // If RA sends Replica as dropped then FM is free to put it in Processed List
        if (failoverUnit->IsClosed && context.FTInfo.LocalReplica.State == Reliability::ReplicaStates::Dropped)
        {
            CloseLocalReplica(handlerParameters, ReplicaCloseMode::Delete, ActivityDescription::Empty);
        }
    }    

    return true;
}

#pragma region Message Sending Utility Code

#pragma region Message send to RA code

void ReconfigurationAgent::AddActionSendMessageToRA(
    StateMachineActionQueue & actionQueue,
    RSMessage const & msgType,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & fuDesc, 
    ReplicaDescription const & replicaDesc,
    ServiceDescription const & serviceDesc)
{
    ReplicaMessageBody body(fuDesc, replicaDesc, serviceDesc);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAAction<ReplicaMessageBody>>(
            msgType,
            nodeInstance,
            move(body)));
}

void ReconfigurationAgent::AddActionSendContinueSwapPrimaryMessageToRA(
    StateMachineActionQueue & actionQueue,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & fuDesc, 
    ServiceDescription const & serviceDesc,
    vector<ReplicaDescription> && replicaDescList,
    TimeSpan phase0Duration)
{
    DoReconfigurationMessageBody body(fuDesc, serviceDesc, move(replicaDescList), phase0Duration);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAAction<DoReconfigurationMessageBody>>(
            RSMessage::GetDoReconfiguration(),
            nodeInstance,
            move(body)));
}

void ReconfigurationAgent::AddActionSendCreateReplicaMessageToRA(
    Infrastructure::StateMachineActionQueue & actionQueue,
    Federation::NodeInstance const nodeInstance,
    Reliability::FailoverUnitDescription const & fuDesc,
    Reliability::ReplicaDescription const & replicaDesc,
    Reliability::ServiceDescription const & serviceDesc,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo)
{
    RAReplicaMessageBody body(fuDesc, replicaDesc, serviceDesc, deactivationInfo);

    actionQueue.Enqueue(
        make_unique<SendMessageToRAAction<RAReplicaMessageBody>>(
            RSMessage::GetCreateReplica(),
            nodeInstance,
            move(body)));
}

void ReconfigurationAgent::AddActionSendActivateMessageToRA(
    StateMachineActionQueue & actionQueue,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & fuDesc,
    ServiceDescription const & serviceDesc,
    vector<ReplicaDescription> && replicaDescList,
    ReplicaDeactivationInfo const & deactivationInfo)
{
    ActivateMessageBody body(fuDesc, serviceDesc, move(replicaDescList), deactivationInfo);

    actionQueue.Enqueue(
        make_unique<SendMessageToRAAction<ActivateMessageBody>>(
        RSMessage::GetActivate(),
        nodeInstance,
        move(body)));
}

void ReconfigurationAgent::AddActionSendDeactivateMessageToRA(
    StateMachineActionQueue & actionQueue,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & fuDesc, 
    ServiceDescription const & serviceDesc,
    vector<ReplicaDescription> && replicaDescList,
    ReplicaDeactivationInfo const & deactivationInfo,
    bool isForce)
{
    DeactivateMessageBody body(fuDesc, serviceDesc, move(replicaDescList), deactivationInfo, isForce);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAAction<DeactivateMessageBody>>(
            RSMessage::GetDeactivate(),
            nodeInstance,
            move(body)));
}

void ReconfigurationAgent::AddActionSendMessageReplyToRA(
    StateMachineActionQueue & actionQueue,
    RSMessage const & msgType,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & fuDesc,
    ReplicaDescription const & replicaDesc,
    ErrorCodeValue::Enum errCodeValue)
{
    ReplicaReplyMessageBody body(fuDesc, replicaDesc, ErrorCode(errCodeValue));

    AddActionSendMessageReplyToRA(actionQueue, msgType, nodeInstance, move(body));
}

void ReconfigurationAgent::AddActionSendMessageReplyToRA(
    Infrastructure::StateMachineActionQueue & actionQueue,
    RSMessage const & message,
    Federation::NodeInstance const nodeInstance,
    ReplicaReplyMessageBody && reply)
{
    actionQueue.Enqueue(
        make_unique<SendMessageToRAAction<ReplicaReplyMessageBody>>(
            message,
            nodeInstance,
            move(reply)));
}

void ReconfigurationAgent::AddActionSendMessageReplyToRA(
    StateMachineActionQueue & actionQueue,
    RSMessage const & msgType,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & fuDesc, 
    vector<ReplicaDescription> && replicaDescList,
    ErrorCodeValue::Enum  errCodeValue)
{
    ConfigurationReplyMessageBody body(fuDesc, move(replicaDescList), ErrorCode(errCodeValue));

    actionQueue.Enqueue(
         make_unique<SendMessageToRAAction<ConfigurationReplyMessageBody>>(
            msgType,
            nodeInstance,
            move(body)));
}

void ReconfigurationAgent::AddActionSendGetLsnReplicaNotFoundMessageReplyToRA(
    Infrastructure::StateMachineActionQueue & actionQueue,
    Federation::NodeInstance const nodeInstance,
    ReplicaMessageBody const & getLSNBody)
{
    AddActionSendGetLsnMessageReplyToRA(
        actionQueue, 
        nodeInstance,
        getLSNBody.FailoverUnitDescription, 
        getLSNBody.ReplicaDescription, 
        ReplicaDeactivationInfo(), 
        ErrorCodeValue::NotFound);
}

void ReconfigurationAgent::AddActionSendGetLsnMessageReplyToRA(
    Infrastructure::StateMachineActionQueue & actionQueue,
    Federation::NodeInstance const nodeInstance,
    FailoverUnitDescription const & ftDesc,
    Reliability::ReplicaDescription const & replicaDesc,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo,
    Common::ErrorCodeValue::Enum  errCodeValue)
{
    GetLSNReplyMessageBody body(ftDesc, replicaDesc, deactivationInfo, errCodeValue);

    actionQueue.Enqueue(
        make_unique<SendMessageToRAAction<GetLSNReplyMessageBody>>(
            RSMessage::GetGetLSNReply(),
            nodeInstance,
            move(body)));
}

#pragma endregion

#pragma region Message send to RAP

bool IsValidServiceTypeRegistration(Hosting2::ServiceTypeRegistrationSPtr const & registration)
{
    TESTASSERT_IF(registration == nullptr, "Registration cannot be null");

    // This should not happen - guarded by the assert above
    // The assert above is disabled in production, ignore the request to send this message to RAP
    // Bringing down the node is bad
    return registration != nullptr;
}

// Send request message to RA-Proxy
void ReconfigurationAgent::AddActionSendMessageToRAProxy(
    StateMachineActionQueue & actionQueue,
    RAMessage const & msgType,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    FailoverUnitDescription const & fuDesc,
    ReplicaDescription const & localReplicaDesc,
    ServiceDescription const & serviceDesc,
    ProxyMessageFlags::Enum flags)
{
    if (!IsValidServiceTypeRegistration(registration))
    {
        return;
    }

    ProxyRequestMessageBody body(registration->RuntimeId, fuDesc, localReplicaDesc, serviceDesc, flags);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAPAction<ProxyRequestMessageBody>>(
            msgType,
            registration,
            move(body)));
}

// Send request message to RA-Proxy
void ReconfigurationAgent::AddActionSendMessageToRAProxy(
    StateMachineActionQueue & actionQueue,
    RAMessage const & msgType,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    FailoverUnitDescription const & fuDesc,
    ReplicaDescription const & localReplicaDesc,
    ProxyMessageFlags::Enum flags)
{
    if (!IsValidServiceTypeRegistration(registration))
    {
        return;
    }

    ProxyRequestMessageBody body(registration->RuntimeId, fuDesc, localReplicaDesc, flags);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAPAction<ProxyRequestMessageBody>>(
            msgType,
            registration,
            move(body)));
}

// Send request message to RA-Proxy
void ReconfigurationAgent::AddActionSendMessageToRAProxy(
    StateMachineActionQueue & actionQueue,
    RAMessage const & msgType,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    FailoverUnitDescription const & fuDesc,
    ReplicaDescription const & localReplicaDesc,
    ReplicaDescription const & remoteReplicaDesc)
{
    if (!IsValidServiceTypeRegistration(registration))
    {
        return;
    }

    ProxyRequestMessageBody body(fuDesc, localReplicaDesc, remoteReplicaDesc);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAPAction<ProxyRequestMessageBody>>(
            msgType,
            registration,
            move(body)));
}

// Send request message to RA-Proxy
void ReconfigurationAgent::AddActionSendMessageToRAProxy(
    StateMachineActionQueue & actionQueue,
    RAMessage const & msgType,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    FailoverUnitDescription const & fuDesc,
    Reliability::ReplicaDescription localReplicaDesc,
    vector<ReplicaDescription> && replicaDescList,
    ProxyMessageFlags::Enum flags)
{
    ProxyRequestMessageBody body(fuDesc, localReplicaDesc, move(replicaDescList), flags);

    actionQueue.Enqueue(
         make_unique<SendMessageToRAPAction<ProxyRequestMessageBody>>(
            msgType,
            registration,
            move(body)));
}

#pragma endregion

#pragma endregion

void ReconfigurationAgent::ProcessIpcMessage(
    IMessageMetadata const * metadata,
    MessageUPtr && messagePtr, 
    IpcReceiverContextUPtr && context)
{
    messageHandler_.ProcessIpcMessage(metadata, move(messagePtr), move(context));
}

void ReconfigurationAgent::ReportLoadMessageHandler(Message & message)
{
    ReportLoadMessageBody body;
    if (!TryGetMessageBody(NodeInstanceIdStr, message, body))
    {
        return;
    }

    ProcessLoadReport(body);
}

void ReconfigurationAgent::ReportHealthReportMessageHandler(MessageUPtr && messagePtr, IpcReceiverContextUPtr && context)
{
    // Just send the report to HM
    HealthSubsystem.SendToHealthManager(move(messagePtr), move(context));
}

void ReconfigurationAgent::ReportResourceUsageHandler(Message & message)
{
    Management::ResourceMonitor::ResourceUsageReport body;
    if (!TryGetMessageBody(NodeInstanceIdStr, message, body))
    {
        return;
    }

    resourceMonitorComponent_->ProcessResoureUsageMessage(body);
}

void ReconfigurationAgent::ProcessLoadReport(ReportLoadMessageBody const & loadReport)
{
    MessageUPtr msg = RSMessage::GetReportLoad().CreateMessage<ReportLoadMessageBody>(move(loadReport));

    federationWrapper_->SendToFM(move(msg));
}

void ReconfigurationAgent::ProxyReplicaEndpointUpdatedMessageHandler(HandlerParameters & parameters, ReplicaMessageContext & msgContext)
{
    auto & ft = parameters.FailoverUnit;

    ft->ProcessProxyReplicaEndpointUpdate(msgContext.Body, parameters.ExecutionContext);
}

void ReconfigurationAgent::ReadWriteStatusRevokedNotificationMessageHandler(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & msgContext)
{
    auto & ft = parameters.FailoverUnit;

    ft->ProcessReadWriteStatusRevokedNotification(msgContext.Body, parameters.ExecutionContext);
}

void ReconfigurationAgent::ReportFaultMessageHandler(HandlerParameters & parameters, ReportFaultMessageContext & msgContext)
{
    parameters.AssertFTIsOpen();

    ReportFaultHandler(parameters, msgContext.Body.FaultType, msgContext.Body.ActivityDescription);
}

void ReconfigurationAgent::ReportFaultHandler(HandlerParameters & handlerParameters, FaultType::Enum faultType, ActivityDescription const & activityDescription)
{
    auto & failoverUnit = handlerParameters.FailoverUnit;

    if (failoverUnit->LocalReplicaClosePending.IsSet)
    {
        return;
    }

    ReplicaCloseMode closeMode = ReplicaCloseMode::None;
    switch (faultType)
    {
    case FaultType::Permanent:
        closeMode = ReplicaCloseMode::Abort;
        break;

    case FaultType::Transient:
        closeMode = failoverUnit->HasPersistedState ? ReplicaCloseMode::Restart : ReplicaCloseMode::Drop;
        break;

    default:
        // Do not assert here - we could add a new fault type in the future
        return;
    };

    CloseLocalReplica(handlerParameters, closeMode, activityDescription);
}

void ReconfigurationAgent::StatefulServiceReopenReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    ReplicaOpenReplyHandler(parameters, msgContext);
}

void ReconfigurationAgent::ReplicaOpenReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    // Get the locked FailoverUnit
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    
    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    if (failoverUnit->ServiceTypeRegistration == nullptr)
    {
        return;
    }
    
    if (!failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        return;
    }

    // Get the local replica
    Replica & localReplica = failoverUnit->LocalReplica;

    if (localReplica.IsInvalid ||
        localReplica.InstanceId != msgBody.LocalReplicaDescription.InstanceId)
    {
        return;
    }

    if (localReplica.State != ReplicaStates::InCreate && localReplica.State != ReplicaStates::StandBy)
    {
        return;
    }

    if (localReplica.State == ReplicaStates::InCreate && msgBody.LocalReplicaDescription.State == Reliability::ReplicaStates::StandBy)
    {
        // Reopen Reply received after the same replica is IC due to a create replica or add primary
        return;
    }

    if (failoverUnit->RetryableErrorStateObj.IsLastRetryBeforeDrop(Config) && 
        msgBody.Flags != ProxyMessageFlags::Abort)
    {
        //Stale message. RAP will echo the abort message with an abort
        return;
    }

    TESTASSERT_IF(!failoverUnit->RetryableErrorStateObj.IsLastRetryBeforeDrop(Config) && msgBody.Flags == ProxyMessageFlags::Abort, "RA must be at LastRetryBeforeDrop or RAP cannot send message with this flag {0} \r\n{1}", *failoverUnit, msgBody);
    
    ReplicaOpenReplyHandlerHelper(failoverUnit, msgBody.LocalReplicaDescription, TransitionErrorCode(msgBody.ErrorCode), parameters);
}

void ReconfigurationAgent::ReplicaOpenReplyHandlerHelper(
    Infrastructure::LockedFailoverUnitPtr & failoverUnit,
    Reliability::ReplicaDescription const & replicaDescription,
    TransitionErrorCode const & transitionErrorCode,
    Infrastructure::HandlerParameters & parameters)
{
    // capture state that may no longer be valid after ProcessReplicaOpenreply
    auto senderNode = failoverUnit->SenderNode;
    auto openMode = failoverUnit->OpenMode;
    auto preReplicaDescription = failoverUnit->LocalReplica.ReplicaDescription;

    failoverUnit.EnableUpdate();
    failoverUnit->ProcessReplicaOpenReply(
        Config,
        replicaDescription,
        transitionErrorCode,
        parameters.ExecutionContext);

    auto errorValue = transitionErrorCode.ReadValue();

    if (errorValue == ErrorCodeValue::RAProxyStateChangedOnDataLoss)
    {
        // State changed on data loss is an internal error between RA/RAP
        errorValue = ErrorCodeValue::Success;
    }

    if (failoverUnit->IsClosed)
    {
        // The replica was dropped
        // Finish the open processing with an error
        // Use the captured replica description here
        FinishOpenLocalReplica(failoverUnit, parameters, openMode, senderNode, preReplicaDescription, errorValue);
    }
    else if (failoverUnit->LocalReplicaClosePending.IsSet)
    {
        ProcessReplicaCloseMessageRetry(parameters);
    }
    else if (failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        // If the node is deactivated short circuit all further processing and close
        if (!nodeState_->GetNodeDeactivationState(failoverUnit->Owner).IsActivated)
        {
            CloseLocalReplica(parameters, ReplicaCloseMode::DeactivateNode, ActivityDescription::Empty);
            return;
        }
    }
    else if (!failoverUnit->LocalReplicaOpenPending.IsSet)
    {
        // During addprimary the RA goes from {} to {P/N/P IB U 1:1} 
        // Onece add primary completes RA needs to reset PC Role and PC Epoch
        // This should only happen during AddPrimary and not reopen of existing primary
        if (errorValue == ErrorCodeValue::Success &&
            failoverUnit->LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary &&
            openMode != RAReplicaOpenMode::Reopen)
        {
            failoverUnit->LocalReplica.PreviousConfigurationRole = Reliability::ReplicaRole::None;
            failoverUnit->PreviousConfigurationEpoch = Epoch::InvalidEpoch();
        }

        if (!nodeState_->GetNodeDeactivationState(failoverUnit->Owner).IsActivated)
        {
            CloseLocalReplica(parameters, ReplicaCloseMode::DeactivateNode, ActivityDescription::Empty);
            return;
        }

        // Open is complete
        // Use the ReplicaDescription on the FT here
        TESTASSERT_IF(failoverUnit->IsReconfiguring, "FT cannot be reconfiguring when it receives ReplicaOpenReply {0}", *failoverUnit);
        FinishOpenLocalReplica(failoverUnit, parameters, openMode, senderNode, failoverUnit->LocalReplica.ReplicaDescription, errorValue);
    }
}

void ReconfigurationAgent::ReplicaCloseReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    // Get the locked FailoverUnit
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    if (failoverUnit->IsClosed || !failoverUnit->LocalReplica.IsUp)
    {
        return;
    }

    if (failoverUnit->LocalReplicaId != msgBody.LocalReplicaDescription.ReplicaId ||
        failoverUnit->LocalReplicaInstanceId != msgBody.LocalReplicaDescription.InstanceId)
    {
        return;
    }

    if (!failoverUnit->LocalReplicaClosePending.IsSet)
    {
        return;
    }

    ReplicaCloseReplyHandlerHelper(failoverUnit, TransitionErrorCode(msgBody.ErrorCode), parameters);
}

void ReconfigurationAgent::ReplicaCloseReplyHandlerHelper(
    Infrastructure::LockedFailoverUnitPtr & failoverUnit,
    TransitionErrorCode const & transitionErrorCode,
    Infrastructure::HandlerParameters & parameters)
{
    // Capture variables 
    auto closeMode = failoverUnit->CloseMode;
    auto senderNode = failoverUnit->SenderNode;

    failoverUnit->ProcessReplicaCloseReply(
        transitionErrorCode,
        parameters.ExecutionContext);

    if (failoverUnit->IsLocalReplicaClosed(closeMode))
    {
        TESTASSERT_IF(!failoverUnit->IsLocalReplicaClosed(closeMode), "Currently close must succeed {0} {1}", closeMode, *failoverUnit);
        FinishCloseLocalReplica(parameters, closeMode, senderNode);
    }
}

void ReconfigurationAgent::UpdateConfigurationReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    // Get the locked FailoverUnit
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    ErrorCodeValue::Enum errorCodeValue = msgBody.ErrorCode.ReadValue();

    failoverUnit->ProcessReplicationConfigurationUpdate(msgBody);

    // Was it an only update configuration
    if (msgBody.Flags == ProxyMessageFlags::None)
    {
        if (errorCodeValue == ErrorCodeValue::Success)
        {
            /*
                Consider I/S promotion here as well as Stale UC reply during Phase0_Demote
            */
            if (failoverUnit->LocalReplica.PreviousConfigurationRole != ReplicaRole::None && 
                failoverUnit->LocalReplica.CurrentConfigurationRole == ReplicaRole::Secondary &&
                !failoverUnit->IsReconfiguring &&
                failoverUnit->SenderNode != ReconfigurationAgent::InvalidNode)
            {
                // failoverUnit is being updated 
                failoverUnit.EnableUpdate();

                auto sender = failoverUnit->SenderNode;
                failoverUnit->FinishActivate(
                    msgBody.LocalReplicaDescription, 
                    true /*updateEndpoints*/,
                    parameters.ExecutionContext);

                AddActionSendMessageReplyToRA(
                    parameters.ActionQueue,
                    RSMessage::GetActivateReply(),
                    sender,
                    failoverUnit->FailoverUnitDescription,
                    failoverUnit->LocalReplica.ReplicaDescription,
                    errorCodeValue);
            }
            else if (failoverUnit->IsReconfiguring)
            {
                /*
                    A UC Reply with no flags can complete the reconfiguration
                    Consider that the FT is in Phase4Activate and is waiting for one replica to be built
                    Write status has already been granted so the UCReply with ER has been received
                    At this time RA will have Phase4Activate and only UpdateReplicatorConfiguration pending

                    RA will send UC to replicator to update its view and the reply will have no flags
                    The state will be updated above

                    RA should check the reconfig progress and see if it can complete it at this time
                */
                failoverUnit->CheckReconfigurationProgressAndHealthReportStatus(parameters.ExecutionContext);
            }
        }
    }
    // Was it for EndReconfiguration during Reconfiguration sent after Phase3_DeactivateWriteQuorum
    else if (msgBody.Flags == ProxyMessageFlags::EndReconfiguration)
    {
        if (errorCodeValue == ErrorCodeValue::Success && failoverUnit->ReconfigurationStage == FailoverUnitReconfigurationStage::Phase4_Activate)
        {
            failoverUnit.EnableUpdate();
            
            failoverUnit->ReplicatorConfigurationUpdateAfterDeactivateCompleted(parameters.ExecutionContext);
        }
    }    
    else if (msgBody.Flags == ProxyMessageFlags::Catchup)
    {
        TESTASSERT_IF(failoverUnit->CurrentConfigurationEpoch != msgBody.FailoverUnitDescription.CurrentConfigurationEpoch, "incoming epoch not equal to ft epoch {0} {1}", msgBody, *failoverUnit);
        
        failoverUnit->ProcessCatchupCompletedReply(msgBody.ErrorCode, msgBody.LocalReplicaDescription, parameters.ExecutionContext);
    }
    else
    {
        Assert::TestAssert("unknown flags {0}", msgBody);
    }
}

void ReconfigurationAgent::ReplicatorBuildIdleReplicaReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    // Get the locked FailoverUnit
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    TESTASSERT_IF(msgBody.ErrorCode.IsError(ErrorCodeValue::RAProxyDemoteCompleted), "RAP should not send this error but just drop the request {0} {1}", msgBody, *failoverUnit);

    if (failoverUnit->LocalReplicaRole != ReplicaRole::Primary && 
        !failoverUnit->AllowBuildDuringSwapPrimary)
    {
        // FailoverUnit has undergone reconfiguration before the reply was received,
        // just ignore message
        return;
    }

    // Get the replica
    Replica & replica = failoverUnit->GetReplica(msgBody.RemoteReplicaDescription.FederationNodeId);

    if (replica.IsInvalid || 
        replica.InstanceId != msgBody.RemoteReplicaDescription.InstanceId ||
        !replica.IsUp)
    {
        // Replica state has changed, ignore stale message
        return;
    }

    if (!replica.IsInBuild)
    {
        return;
    }

    bool sendToFM = !failoverUnit->IsReconfiguring;

    if (msgBody.ErrorCode.IsSuccess())
    {
        // FailoverUnit is being updated
        failoverUnit.EnableUpdate();
                           
        failoverUnit->FinishBuildIdleReplica(replica, parameters.ExecutionContext, sendToFM);
    }
    else if (msgBody.ErrorCode.IsError(ErrorCodeValue::REReplicaAlreadyExists))
    {
        // This is hit due to out of order UpdateConfiguration message in RAP
        // suppress the reply for FM to retry
        failoverUnit->IsUpdateReplicatorConfiguration = true;

        sendToFM = false;
    }    
    else
    {
        return;
    }

    if (sendToFM)
    {
        // Send response back to the FM

        ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, replica.ReplicaDescription, msgBody.ErrorCode.ReadValue());

        failoverUnit->TransportUtility.AddSendMessageToFMAction(
            parameters.ActionQueue,
            RSMessage::GetAddReplicaReply(),
            move(body));
    }
}

void ReconfigurationAgent::ReplicatorRemoveIdleReplicaReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    // Get the locked FailoverUnit
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    if (!msgBody.ErrorCode.IsSuccess())
    {
        return;
    }

    if (failoverUnit->LocalReplicaRole != ReplicaRole::Primary && !failoverUnit->IsReconfiguring)
    {
        // Failover unit has undergone reconfiguration before the reply was received,
        // just ignore message
        return;
    }

    // Get the replica
    Replica & replica = failoverUnit->GetReplica(msgBody.RemoteReplicaDescription.FederationNodeId);

    if (replica.IsInvalid || 
        (!replica.IsInConfiguration && !replica.IsInDrop) ||
        replica.InstanceId > msgBody.RemoteReplicaDescription.InstanceId)
    {
        // Replica state has changed, ignore stale message
        return;
    }

    if (replica.IsInConfiguration && replica.InstanceId == msgBody.RemoteReplicaDescription.InstanceId)
    {
        // For replica in the configuration, just reset the message stage after the replica has been removed from replicator, 
        // the replica will be removed from the configuration by reconfiguration

        // Reset flag since this replica has been removed from replicator view
        replica.ReplicatorRemovePending = false;

        return;
    }

    ASSERT_IFNOT(replica.IsInDrop,
        "Invalid replica state on reply from RAP in remove idle replica: {0} msg:{1}", 
        *(failoverUnit), msgBody);
        
    // FailoverUnit is being updated
    failoverUnit.EnableUpdate();
        
    // Remove the Idle replica
    failoverUnit->FinishRemoveIdleReplica(replica);

    // Send reply back to FM
    ReplicaReplyMessageBody body(failoverUnit->FailoverUnitDescription, msgBody.RemoteReplicaDescription, ErrorCodeValue::Success);
    failoverUnit->TransportUtility.AddSendMessageToFMAction(
        parameters.ActionQueue,
        RSMessage::GetRemoveReplicaReply(),
        move(body));    
}

void ReconfigurationAgent::ReplicatorGetStatusMessageHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    // Get the locked FailoverUnit
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;
    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    if (failoverUnit->LocalReplica.InstanceId != msgBody.LocalReplicaDescription.InstanceId)
    {
        return;
    }

    if (!msgBody.ErrorCode.IsSuccess())
    {
        return;
    }

    TESTASSERT_IF(
        failoverUnit->CurrentConfigurationEpoch.ToPrimaryEpoch() != msgBody.FailoverUnitDescription.CurrentConfigurationEpoch.ToPrimaryEpoch(),
        "Epoch mismatch at get lsn reply - should be guarded by message context staleness check {0} \r\n {1}",
        msgBody,
        *(failoverUnit));

    failoverUnit->ProcessReplicatorGetStatusReply(msgBody.LocalReplicaDescription, parameters.ExecutionContext);
}

void ReconfigurationAgent::ReplicatorCancelCatchupReplicaSetReplyHandler(HandlerParameters & parameters, ProxyReplyMessageContext & msgContext)
{
    LockedFailoverUnitPtr & failoverUnit = parameters.FailoverUnit;

    ProxyReplyMessageBody const & msgBody = msgContext.Body;

    failoverUnit->ProcessCancelCatchupReply(
        msgBody.ErrorCode,
        msgBody.FailoverUnitDescription,
        parameters.ExecutionContext);
}

GenerationProposalReplyMessageBody ReconfigurationAgent::ProcessGenerationProposal(Message & request, bool & isGfumUploadOut)
{
    GenerationProposalReplyMessageBody body = GenerationStateManagerObj.ProcessGenerationProposal(request, isGfumUploadOut);

    if (!IsOpen)
    {
        body.Error = ErrorCodeValue::RANotReadyForUse;
    }

    return body;
}

void ReconfigurationAgent::ProcessGenerationUpdate(Transport::Message & request, Federation::NodeInstance const & from)
{
    GenerationHeader generationHeader = GenerationHeader::ReadFromMessage(request);

    // The message body is only set if the request is from the FM and not FMM
    GenerationUpdateMessageBody body;    
    if (!generationHeader.IsForFMReplica && !request.GetBody<GenerationUpdateMessageBody>(body))
    {
        WriteWarning(GenerationType, "RA on node {0} dropped GenerationUpdate message because deserialization of body failed {1}", NodeInstance, request.Status);
        return;
    }

    return GenerationStateManagerObj.ProcessGenerationUpdate(wformatString(request.MessageId), generationHeader, body, from);
}

void ReconfigurationAgent::ProcessLFUMUploadReply(GenerationHeader const & generationHeader)
{
    GenerationStateManagerObj.ProcessLFUMUploadReply(generationHeader);
}

bool ReconfigurationAgent::TryDropFMReplica(HandlerParameters & handlerParameters, ReplicaMessageBody const & body)
{
    auto & failoverUnit = handlerParameters.FailoverUnit;
    if (!body.FailoverUnitDescription.FailoverUnitId.IsFM)
    {
        return false;
    }

    // Important to validate these here because this is called from AddPrimary/CreateReplica before creating the FT object
    if (!failoverUnit || failoverUnit->IsClosed)
    {
        return false;
    }

    if (failoverUnit->LocalReplica.ReplicaId < body.ReplicaDescription.ReplicaId &&
        failoverUnit->CurrentConfigurationEpoch < body.FailoverUnitDescription.CurrentConfigurationEpoch)
    {
        CloseLocalReplica(handlerParameters, ReplicaCloseMode::ForceAbort, ActivityDescription::Empty);
        return true;
    }

    return false;
}

MultipleEntityBackgroundWorkManager & ReconfigurationAgent::GetMultipleFailoverUnitBackgroundWorkManager(EntitySetName::Enum setName)
{
    switch (setName)
    {
    case EntitySetName::ReconfigurationMessageRetry:
        return *reconfigurationMessageRetryWorkManager_;

    case EntitySetName::StateCleanup:
        return *stateCleanupWorkManager_;

    case EntitySetName::ReplicaCloseMessageRetry:
        return *replicaCloseMessageRetryWorkManager_;

    case EntitySetName::ReplicaOpenMessageRetry:
        return *replicaOpenWorkManager_;

    case EntitySetName::UpdateServiceDescriptionMessageRetry:
        return *updateServiceDescriptionMessageRetryWorkManager_;

    default:
        Assert::CodingError("Unknown set name {0}", static_cast<int>(setName));
    };        
}

EntitySet & ReconfigurationAgent::GetSet(EntitySetName::Enum setName)
{
    return entitySets_.Get(EntitySetIdentifier(setName)).Set;
}

RetryTimer & ReconfigurationAgent::GetRetryTimer(EntitySetName::Enum setName)
{
    return *entitySets_.Get(EntitySetIdentifier(setName)).Timer;
}

bool ReconfigurationAgent::TryCreateFailoverUnit(
    ReplicaMessageBody const & msgBody,
    LockedFailoverUnitPtr & ft)
{
    if (!ft)
    {
        ft.Insert(make_shared<FailoverUnit>(
            Config, 
            msgBody.FailoverUnitDescription.FailoverUnitId,
            msgBody.FailoverUnitDescription.ConsistencyUnitDescription,
            msgBody.ServiceDescription));
        return true;
    }

    return false;
}

bool ReconfigurationAgent::TryCreateFailoverUnit(
    DoReconfigurationMessageBody const & msgBody,
    LockedFailoverUnitPtr & ft)
{
    if (!ft)
    {
        ft.Insert(make_shared<FailoverUnit>(
            Config,
            msgBody.FailoverUnitDescription.FailoverUnitId,
            msgBody.FailoverUnitDescription.ConsistencyUnitDescription,
            msgBody.ServiceDescription));
        return true;
    }

    return false;
}

bool ReconfigurationAgent::CheckIfAddFailoverUnitMessageCanBeProcessed(
    Reliability::ReplicaMessageBody const & msgBody,
    std::function<void(ReplicaReplyMessageBody)> const & replyFunc)
{
    /*
        Reject messages that create new FTs on a node if the node is either going through fabric upgrading or deactivated

        This is because once the node is in these states it has already snapshotted the LFUM and will not know
        that new replicas have been added and these also need to be closed

        Ideally, the FM should not place the replica on the node and this code is just defence in depth

        The RA should send back an error to the FM so that the FM also removes the replica

        Due to FM/PLB bug sending back an error in fabric upgrade results in a very tight loop between FM, RA, PLB where 
        replica is placed and dropped. After that is fixed then send back error even in fabric upgrade
    */
    if (StateObj.IsUpgrading)
    {
        return false;
    }

    if (!NodeStateObj.GetNodeDeactivationState(FailoverManagerId::Get(msgBody.FailoverUnitDescription.FailoverUnitId)).IsActivated)
    {
        ReplicaReplyMessageBody body(msgBody.FailoverUnitDescription, msgBody.ReplicaDescription, ErrorCodeValue::RANotReadyForUse);
        replyFunc(move(body));
        return false;
    }

    return true;
}

bool ReconfigurationAgent::AnyReplicaFound(bool excludeFMServiceReplica)
{
    bool anyReplicaFound = !LocalFailoverUnitMapObj.IsEmpty();

    if (excludeFMServiceReplica &&
        LocalFailoverUnitMapObj.GetCount() == 1u &&
        LocalFailoverUnitMapObj.GetEntry(FailoverUnitId(Constants::FMServiceGuid)))
    {
        anyReplicaFound = false;
    }

    return anyReplicaFound;
}

bool ReconfigurationAgent::TryUpdateServiceDescription(LockedFailoverUnitPtr & failoverUnit, StateMachineActionQueue & queue, ServiceDescription const & newServiceDescription)
{
    if (!failoverUnit)
    {
        return false;
    }

    if (!failoverUnit->ValidateUpdateServiceDescription(newServiceDescription))
    {
        return false;
    }

    failoverUnit.EnableUpdate();
    failoverUnit->UpdateServiceDescription(queue, newServiceDescription);
    return true;
}

ReconfigurationAgent::State::State(ReconfigurationAgent & ra)
: ra_(ra),
  isOpen_(false),
  isClosing_(false),
  nodeUpAckFromFmmProcessed_(false),
  nodeUpAckFromFMProcessed_(false),
  isUpgrading_(false)
{
}

void ReconfigurationAgent::State::OnOpenBegin()
{
    RAEventSource::Events->LifeCycleOpen(ra_.NodeInstanceIdStr);
    ASSERT_IF(IsOpen, "Ra cannot be open on node {0}", ra_.NodeInstance);
}

void ReconfigurationAgent::State::OnOpenComplete()
{
    RAEventSource::Events->LifeCycleOpenSuccess(ra_.NodeInstanceIdStr, ra_.LocalFailoverUnitMapObj.GetCount());

    AcquireWriteLock grab(lock_);
    AssertInvariants();
    isOpen_ = true;
}

void ReconfigurationAgent::State::OnNodeUpAckProcessed(Reliability::FailoverManagerId const & fm)
{
    AcquireWriteLock grab(lock_);
    AssertInvariants();
    if (!isOpen_)
    {
        return;
    }

    if (!fm.IsFmm)
    {
        RAEventSource::Events->LifeCycleNodeUpAckProcessed(ra_.NodeInstanceIdStr, FMSource);
        nodeUpAckFromFMProcessed_ = true;
    }
    else
    {
        RAEventSource::Events->LifeCycleNodeUpAckProcessed(ra_.NodeInstanceIdStr, FMMSource);
        nodeUpAckFromFmmProcessed_ = true;
    }
}

void ReconfigurationAgent::State::OnCloseBegin()
{
    RAEventSource::Events->LifeCycleCloseBegin(ra_.NodeInstanceIdStr);

    AcquireWriteLock grab(lock_);
    AssertInvariants();
    isOpen_ = false;
    isClosing_ = true;
}

void ReconfigurationAgent::State::OnReplicasClosed()
{
    AcquireWriteLock grab(lock_);
    AssertInvariants();
    isClosing_ = false;
    nodeUpAckFromFmmProcessed_ = false;
    nodeUpAckFromFMProcessed_ = false;
}

void ReconfigurationAgent::State::OnCloseComplete()
{
    RAEventSource::Events->LifeCycleCloseEnd(ra_.NodeInstanceIdStr);
}

void ReconfigurationAgent::State::OnFabricCodeUpgradeStart()
{
    RAEventSource::Events->LifeCycleNodeUpgradingStateChange(ra_.NodeInstanceIdStr, true);

    AcquireWriteLock grab(lock_);
    ASSERT_IF(isUpgrading_, "Fabric Upgrade cannot start multiple times {0}", ra_.NodeInstance);
    isUpgrading_ = true;
}

void ReconfigurationAgent::State::OnFabricCodeUpgradeRollback()
{
    RAEventSource::Events->LifeCycleNodeUpgradingStateChange(ra_.NodeInstanceIdStr, false);

    AcquireWriteLock grab(lock_);
    ASSERT_IF(!isUpgrading_, "Fabric Upgrade cannot rollback if not started {0}", ra_.NodeInstance);
    isUpgrading_ = false;
}

void ReconfigurationAgent::State::AssertInvariants() const
{
    ASSERT_IF(!isOpen_ && !isClosing_ && nodeUpAckFromFmmProcessed_, "Node up ack processed while ra is closed at node {0}", ra_.NodeInstance);
    ASSERT_IF(!isOpen_ && !isClosing_ && nodeUpAckFromFMProcessed_, "Node up ack processed while ra is closed at node {0}", ra_.NodeInstance);
    ASSERT_IF(isOpen_ && isClosing_, "Cannot be open and closing {0}", ra_.NodeInstance);
}

bool ReconfigurationAgent::IsReady(FailoverUnitId const & ftId) const
{
    // FM service can be processed once node up ack from Fmm is received
    // Others can only be processed when node up ack from FM is received
    if (ftId.Guid == Constants::FMServiceGuid)
    {
        return state_.IsFmmReady;
    }
    else
    {
        return state_.IsFMReady;
    }
}
