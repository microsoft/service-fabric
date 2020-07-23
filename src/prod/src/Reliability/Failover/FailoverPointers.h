// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This header file contains all shared, unique, weak pointer types well-known in failover
// as well as forward declarations of corresponding types

namespace Reliability
{
    struct ReliabilitySubsystemConstructorParameters;
    class IReliabilitySubsystem;
    typedef std::unique_ptr<IReliabilitySubsystem> IReliabilitySubsystemUPtr;
    
    class ServiceResolver;

    class IFederationWrapper;
    class FederationWrapper;
    class ServiceAdminClient;

    class NodeDescription;

    class ChangeNotificationOperation;
    typedef std::shared_ptr<ChangeNotificationOperation> ChangeNotificationOperationSPtr;

    class ReliabilitySubsystem;
    typedef std::unique_ptr<ReliabilitySubsystem> ReliabilitySubsystemUPtr;

    class NodeUpOperation;
    typedef std::shared_ptr<NodeUpOperation> NodeUpOperationSPtr; 

    class FailoverConfig;
    typedef std::unique_ptr<FailoverConfig> FailoverConfigUPtr;

    class ReplicaDescription;     
    typedef std::vector<ReplicaDescription>::const_iterator ReplicaDescriptionConstIterator;

    class ServiceTableEntry;

    struct ConsistencyUnitId;
    typedef std::unordered_map<std::wstring, std::vector<ConsistencyUnitId>> ServiceCuidMap;

    class UpgradeDescription;

    class NodeUpdateServiceRequestMessageBody;
    typedef std::shared_ptr<NodeUpdateServiceRequestMessageBody> NodeUpdateServiceRequestMessageBodySPtr;

    class ReplicaUpMessageBody;
    typedef std::shared_ptr<ReplicaUpMessageBody> ReplicaUpMessageBodySPtr;

    class NodeUpOperationFactory;

    class LFUMMessageBody;
    class GFUMMessageBody;
    typedef std::unique_ptr<Serialization::IFabricSerializable> SerializedGFUMUPtr;

    struct GenerationHeader;

    namespace FailoverManagerComponent
    {
        class FailoverManagerServiceFactory;
        class FailoverManagerService;

        class FailoverManagerStore;
        typedef std::unique_ptr<FailoverManagerStore> FailoverManagerStoreUPtr;

        class EventSource;

        class FabricUpgradeManager;
        typedef std::unique_ptr<FabricUpgradeManager> FabricUpgradeManagerUPtr;

        class InstrumentedPLB;
        typedef std::unique_ptr<InstrumentedPLB> InstrumentedPLBUPtr;

        class IFailoverManager;
        typedef std::shared_ptr<IFailoverManager> IFailoverManagerSPtr;

        class FailoverManager;
        typedef std::shared_ptr<FailoverManager> FailoverManagerSPtr;

        class TimedRequestReceiverContext;
        typedef std::unique_ptr<TimedRequestReceiverContext> TimedRequestReceiverContextUPtr;

        class FailoverUnit;
        typedef std::unique_ptr<FailoverUnit> FailoverUnitUPtr;
        typedef std::shared_ptr<FailoverUnit> FailoverUnitSPtr;

        class BackgroundThreadContext;
        typedef std::unique_ptr<BackgroundThreadContext> BackgroundThreadContextUPtr;

        class BackgroundManager;
        typedef std::unique_ptr<BackgroundManager> BackgroundManagerUPtr;

        class FailoverUnitCacheEntry;
        typedef std::shared_ptr<FailoverUnitCacheEntry> FailoverUnitCacheEntrySPtr;

        class LockedFailoverUnitPtr;

        class FailoverUnitCache;
        typedef std::unique_ptr<FailoverUnitCache> FailoverUnitCacheUPtr;

        class FailoverUnitMessageProcessor;
        typedef std::unique_ptr<FailoverUnitMessageProcessor> FailoverUnitMessageProcessorUPtr;

        class FailoverUnitJobQueue;
        typedef std::unique_ptr<FailoverUnitJobQueue> FailoverUnitJobQueueUPtr;

        class CommitJobQueue;
        typedef std::unique_ptr<CommitJobQueue> CommitJobQueueUPtr;

        class LockedFailoverUnitPtr;

        class UpgradeDomains;
        typedef std::shared_ptr<UpgradeDomains> UpgradeDomainsSPtr;
        typedef std::shared_ptr<const UpgradeDomains> UpgradeDomainsCSPtr;

        class NodeCache;
        typedef std::unique_ptr<NodeCache> NodeCacheUPtr;

        class NodeInfo;
        typedef std::shared_ptr<NodeInfo> NodeInfoSPtr;

        class LoadCache;
        typedef std::unique_ptr<LoadCache> LoadCacheUPtr;

        class LoadInfo;
        typedef std::shared_ptr<LoadInfo> LoadInfoSPtr;

        class Replica;
        typedef std::vector<Replica>::iterator ReplicaIterator;
        typedef std::vector<Replica>::const_iterator ReplicaConstIterator;
        typedef std::unique_ptr<Replica> ReplicaUPtr;
        typedef std::shared_ptr<Replica> ReplicaSPtr;

        class ServiceCache;
        typedef std::unique_ptr<ServiceCache> ServiceCacheUPtr;

        class ServiceUpdateState;
        typedef std::unique_ptr<ServiceUpdateState> ServiceUpdateStateUPtr;

        class RepartitionInfo;
        typedef std::unique_ptr<RepartitionInfo> RepartitionInfoUPtr;

        class ServiceInfo;
        typedef std::shared_ptr<ServiceInfo> ServiceInfoSPtr;

        class ApplicationInfo;
        typedef std::shared_ptr<ApplicationInfo> ApplicationInfoSPtr;

        template <typename T>
        class Upgrade;

        class FabricUpgrade;
        typedef std::unique_ptr<FabricUpgrade> FabricUpgradeUPtr;

        class ApplicationUpgrade;
        typedef std::unique_ptr<ApplicationUpgrade> ApplicationUpgradeUPtr;

        class UpgradeContext;
        class FabricUpgradeContext;
        class ApplicationUpgradeContext;

        class ServiceType;
        typedef std::shared_ptr<ServiceType> ServiceTypeSPtr;

        class StateMachineAction;
        typedef std::unique_ptr<StateMachineAction> StateMachineActionUPtr;

        class FMQueryHelper;
        typedef std::unique_ptr<FMQueryHelper> FMQueryHelperUPtr;

        class StateMachineTask;
        typedef std::unique_ptr<StateMachineTask> StateMachineTaskUPtr;

        class DynamicStateMachineTask;
        typedef std::unique_ptr<DynamicStateMachineTask> DynamicStateMachineTaskUPtr;

        class InBuildFailoverUnit;
        typedef std::unique_ptr<InBuildFailoverUnit> InBuildFailoverUnitUPtr;
        typedef std::shared_ptr<InBuildFailoverUnit> InBuildFailoverUnitSPtr;

        class InBuildFailoverUnitCache;
        typedef std::unique_ptr<InBuildFailoverUnitCache> InBuildFailoverUnitCacheUPtr;

        typedef Common::CommonTimedJobQueue<FailoverManager> CommonJobQueue;
        typedef std::unique_ptr<CommonJobQueue> CommonJobQueueUPtr;

        class RebuildContext;
        typedef std::unique_ptr<RebuildContext> RebuildContextUPtr;
    }

    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class RAPerformanceCounters;
            typedef std::shared_ptr<RAPerformanceCounters> PerformanceCountersSPtr;

            class IEventWriter;
            typedef std::unique_ptr<IEventWriter> IEventWriterUPtr;
        }

        namespace Infrastructure
        {
            class IRetryPolicy;
            typedef std::shared_ptr<IRetryPolicy> IRetryPolicySPtr;

            class IThrottle;
            typedef std::shared_ptr<IThrottle> IThrottleSPtr;

            class HandlerParameters;

            class EntityJobItemBase;
            typedef std::shared_ptr<EntityJobItemBase> EntityJobItemBaseSPtr;

            class IEntityLock;
            typedef std::unique_ptr<IEntityLock> IEntityLockUPtr;

            class JobItemBase;
            typedef std::unique_ptr<JobItemBase> RAJobItemUPtr;

            class IThreadpool;
            typedef std::unique_ptr<IThreadpool> IThreadpoolUPtr;

            class IEntityMap;

            class EntityEntryBase;
            typedef std::shared_ptr<EntityEntryBase> EntityEntryBaseSPtr;
            typedef std::vector<EntityEntryBaseSPtr> EntityEntryBaseList;
            typedef std::set<Infrastructure::EntityEntryBaseSPtr> EntityEntryBaseSet;

            class IClock;
            typedef std::shared_ptr<IClock> IClockSPtr;

            class RAJobQueueManager;
            typedef std::unique_ptr<RAJobQueueManager> JobQueueManagerUPtr;

            class StateMachineAction;
            typedef std::unique_ptr<StateMachineAction> StateMachineActionUPtr;

            class StateMachineActionQueue;

            class EntitySet;

            class BackgroundWorkManager;
            typedef std::unique_ptr<BackgroundWorkManager> BackgroundWorkManagerUPtr;

            class RetryTimer;

            class BackgroundWorkManagerWithRetry;
            typedef std::unique_ptr<BackgroundWorkManagerWithRetry> BackgroundWorkManagerWithRetryUPtr;

            class MultipleEntityBackgroundWorkManager;
            typedef std::unique_ptr<MultipleEntityBackgroundWorkManager> MultipleEntityBackgroundWorkManagerUPtr;

            class MultipleEntityWork;
            typedef std::shared_ptr<MultipleEntityWork> MultipleEntityWorkSPtr;

            class MultipleEntityWorkManager;
            typedef std::unique_ptr<MultipleEntityWorkManager> MultipleEntityWorkManagerUPtr;

            class ReconfigurationAgentStore;
            typedef std::shared_ptr<ReconfigurationAgentStore> ReconfigurationAgentStoreSPtr;

            class LocalFailoverUnitMap;
            typedef std::unique_ptr<LocalFailoverUnitMap> LocalFailoverUnitMapUPtr;
        }

        namespace Node
        {
            class NodeActivationInfo;

            class NodeDeactivationState;
            typedef std::unique_ptr<NodeDeactivationState> NodeDeactivationStateUPtr;

            class NodeDeactivationStateProcessor;
            typedef std::unique_ptr<NodeDeactivationStateProcessor> NodeDeactivationStateProcessorUPtr;
            
            class NodeState;
            typedef std::unique_ptr<NodeState> NodeStateUPtr;

            class NodeDeactivationMessageProcessor;
            typedef std::unique_ptr<NodeDeactivationMessageProcessor> NodeDeactivationMessageProcessorUPtr;

            class ServiceTypeUpdateProcessor;
            typedef std::unique_ptr<ServiceTypeUpdateProcessor> ServiceTypeUpdateProcessorUPtr;
            
            class PendingReplicaUploadStateProcessor;
            typedef std::unique_ptr<PendingReplicaUploadStateProcessor> PendingReplicaUploadStateProcessorUPtr;

            class FMMessageThrottle;
            typedef std::shared_ptr<FMMessageThrottle> FMMessageThrottleSPtr;
        }

        namespace Query
        {
            class IQueryHandler;
            typedef std::shared_ptr<Query::IQueryHandler> IQueryHandlerSPtr;

            class QueryHelper;
            typedef std::unique_ptr<QueryHelper> QueryHelperUPtr;
        }

        namespace Communication
        {
            class FMTransport;
        }

        namespace Storage
        {
            namespace Api
            {
                class IKeyValueStore;
                typedef std::shared_ptr<IKeyValueStore> IKeyValueStoreSPtr;
            }

            class InMemoryKeyValueStoreState;
            typedef std::shared_ptr<InMemoryKeyValueStoreState> InMemoryKeyValueStoreStateSPtr;
        }

        class IReconfigurationAgent;
        typedef std::unique_ptr<IReconfigurationAgent> IReconfigurationAgentUPtr;

        // RA     

        class ReconfigurationStuckHealthReportDescriptor;
        typedef std::shared_ptr<ReconfigurationStuckHealthReportDescriptor> ReconfigurationStuckDescriptorSPtr;

        class MultipleReplicaCloseCompletionCheckAsyncOperation;
        typedef std::shared_ptr<MultipleReplicaCloseCompletionCheckAsyncOperation> MultipleReplicaCloseCompletionCheckAsyncOperationSPtr;
        typedef std::weak_ptr<MultipleReplicaCloseCompletionCheckAsyncOperation> MultipleReplicaCloseCompletionCheckAsyncOperationWPtr;

        class MultipleReplicaCloseAsyncOperation;
        typedef std::shared_ptr<MultipleReplicaCloseAsyncOperation> MultipleReplicaCloseAsyncOperationSPtr;
        typedef std::weak_ptr<MultipleReplicaCloseAsyncOperation> MultipleReplicaCloseAsyncOperationWPtr;

        class RetryableErrorStateThresholdCollection;
        typedef std::shared_ptr<RetryableErrorStateThresholdCollection> RetryableErrorStateThresholdCollectionSPtr;

        class IRaToRapTransport;
        typedef std::unique_ptr<IRaToRapTransport> IRaToRapTransportUPtr;

        class ReplicaUpMessageSenderBase;
        typedef std::unique_ptr<ReplicaUpMessageSenderBase> ReplicaUpMessageSenderBaseUPtr;

        class NodeUpAckProcessor;
        typedef std::unique_ptr<NodeUpAckProcessor> NodeUpAckProcessorUPtr;

        class ReconfigurationAgent;
        typedef std::unique_ptr<ReconfigurationAgent> ReconfigurationAgentUPtr;

        class FailoverUnit;
        typedef std::shared_ptr<FailoverUnit> FailoverUnitSPtr;

        class Replica;
        typedef std::unique_ptr<Replica> ReplicaUPtr;
        typedef std::shared_ptr<Replica> ReplicaSPtr;

        class Replica;
        typedef std::vector<Replica>::iterator ReplicaIterator;
        typedef std::vector<Replica>::const_iterator ReplicaConstIterator;

        class IReliabilitySubsystemWrapper;
        typedef std::shared_ptr<IReliabilitySubsystemWrapper> IReliabilitySubsystemWrapperSPtr;

        // Define RAP and factory function
        struct ReconfigurationAgentProxyConstructorParameters;
        class IReconfigurationAgentProxy;
        class ReconfigurationAgentProxy;
        typedef std::unique_ptr<IReconfigurationAgentProxy> IReconfigurationAgentProxyUPtr;

        class LFUMUploadOperation;
        typedef std::shared_ptr<LFUMUploadOperation> LFUMUploadOperationSPtr;
       
        // RAP
        class ServiceOperationManager;
        typedef std::unique_ptr<ServiceOperationManager> ServiceOperationManagerUPtr;

        class ReplicatorOperationManager;
        typedef std::unique_ptr<ReplicatorOperationManager> ReplicatorOperationManagerUPtr;

        class FailoverUnitProxy;
        struct ActionListInfo;
        typedef std::shared_ptr<FailoverUnitProxy> FailoverUnitProxySPtr;
        typedef std::weak_ptr<FailoverUnitProxy> FailoverUnitProxyWPtr;

        class ProxyReplyMessageBody;
        typedef std::unique_ptr<ProxyReplyMessageBody> ProxyReplyMessageBodyUPtr;

        class ComProxyStatelessService;
        typedef std::unique_ptr<ComProxyStatelessService> ComProxyStatelessServiceUPtr;

        class ComProxyStatefulService;
        typedef std::unique_ptr<ComProxyStatefulService> ComProxyStatefulServiceUPtr;

        class ComProxyReplicator;
        typedef std::unique_ptr<ComProxyReplicator> ComProxyReplicatorUPtr;

        class ReplicaProxy;

        class ProxyOutgoingMessage;
        typedef std::unique_ptr<ProxyOutgoingMessage> ProxyOutgoingMessageUPtr;

        class LocalFailoverUnitProxyMap;
        typedef std::unique_ptr<LocalFailoverUnitProxyMap> LocalFailoverUnitProxyMapUPtr;

        class LocalLoadReportingComponent;
        typedef std::unique_ptr<LocalLoadReportingComponent> LocalLoadReportingComponentUPtr;

        class LocalMessageSenderComponent;
        typedef std::unique_ptr<LocalMessageSenderComponent> LocalMessageSenderComponentUPtr;

        class LocalHealthReportingComponent;
        typedef std::unique_ptr<LocalHealthReportingComponent> LocalHealthReportingComponentUPtr;

        class JobItemContextBase;
        class ApplicationUpgradeEnumerateFTsJobItemContext;
        class ApplicationUpgradeUpdateVersionJobItemContext;
        class UpgradeCloseCompletionCheckContext;
        class ApplicationUpgradeFinalizeFTJobItemContext;

        namespace Upgrade
        {
            class UpgradeStateDescription;
            typedef std::unique_ptr<UpgradeStateDescription> UpgradeStateDescriptionUPtr;

            class IUpgrade;
            typedef std::unique_ptr<IUpgrade> IUpgradeUPtr;

            class ApplicationUpgrade;
            class FabricUpgrade;

            class UpgradeStateMachine;
            typedef std::shared_ptr<UpgradeStateMachine> UpgradeStateMachineSPtr;

            class UpgradeMessageProcessor;
            typedef std::unique_ptr<UpgradeMessageProcessor> UpgradeMessageProcessorUPtr;

            class ICancelUpgradeContext;
            typedef std::shared_ptr<ICancelUpgradeContext> ICancelUpgradeContextSPtr;

            class RollbackSnapshot;
            typedef std::unique_ptr<RollbackSnapshot> RollbackSnapshotUPtr;

            class FabricCodeVersionProperties;
        }

        namespace Health
        {
            class IHealthSubsystemWrapper;
            typedef std::unique_ptr<IHealthSubsystemWrapper> HealthSubsystemWrapperUPtr;

            class HealthSubsystemWrapper;

            class ReportReplicaHealthStateMachineAction;
        }

        namespace Hosting
        {
            class HostingAdapter;
            typedef std::unique_ptr<HostingAdapter> HostingAdapterUPtr;
        }

        namespace ResourceMonitor
        {
            class ResourceComponent;
            typedef std::unique_ptr<ResourceComponent> ResourceComponentUPtr;

            class ResourceUsageReport;
        }
    }

    namespace ReplicationComponent
    {
        class ComReplicatorFactory;
    }
}

namespace Data
{
    class ComTransactionalReplicatorFactory;
}


namespace Management
{
    namespace NetworkInventoryManager
    {
        class NetworkInventoryService;
        typedef std::unique_ptr<NetworkInventoryService> NetworkInventoryServiceUPtr;
    }
}
