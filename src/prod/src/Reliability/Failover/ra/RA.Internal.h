// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This header contains includes that are private to RA
#include "Reliability/Failover/ra/ra.public.h"
#include "boost/iterator/filter_iterator.hpp"

// Typedefs used only by the RA
typedef Common::ConfigEntry<Common::TimeSpan> TimeSpanConfigEntry;
typedef Common::ConfigEntry<int> IntConfigEntry;
typedef std::function<Reliability::ReconfigurationAgentComponent::Infrastructure::IThreadpoolUPtr (Reliability::ReconfigurationAgentComponent::ReconfigurationAgent &)> ThreadpoolFactory;

#include "Transport/UnreliableTransport.h"
#include "Hosting2/Hosting.Public.h"
#include "Hosting2/Hosting.Runtime.Public.h"

#include "Store/store.h"

#include "Reliability/Replication/Replication.Public.h"
#include "data/txnreplicator/TransactionalReplicator.Public.h"

#include "Reliability/Failover/IFederationWrapper.h"
#include "Reliability/Failover/IReliabilitySubsystem.h"

#include "Reliability/Failover/common/Common.Internal.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"
// TODO: Fix RA to use QueryMessageHandler, rather than directly parsing the query,
// and then remove this header file dependency.
#include "query/QueryMessageUtility.h"

// Structures, Enumerations etc
#include "Reliability/Failover/ra/Upgrade.FabricCodeVersionProperties.h"

// Data Structures
#include "Reliability/Failover/ra/Infrastructure.LookupTableValue.h"
#include "Reliability/Failover/ra/Infrastructure.LookupTable.h"
#include "Reliability/Failover/ra/Infrastructure.LookupTableBuilder.h"

// RA, RAP
#include "Reliability/Failover/ra/CatchupType.h"
#include "Reliability/Failover/ra/UpdateConfigurationReason.h"
#include "Reliability/Failover/ra/ProxyErrorCode.h"
#include "Reliability/Failover/ra/CatchupResult.h"
#include "Reliability/Failover/ra/FailoverUnitTraceEnumerations.h"
#include "Reliability/Failover/ra/FMMessageStage.h"
#include "Reliability/Failover/ra/IReliabilitySubsystemWrapper.h"
#include "Reliability/Failover/ra/ReplicaOpenMode.h"
#include "Reliability/Failover/ra/AccessStatus.h"
#include "Reliability/Failover/ra/AccessStatusType.h"
#include "Reliability/Failover/ra/ProxyActionsListTypes.h"
#include "Reliability/Failover/ra/ProxyActions.h"
#include "Reliability/Failover/ra/ProxyActionsList.h"
#include "Reliability/Failover/ra/ProxyActionsList.ConcurrentExecutionCompatibilityLookupTable.h"
#include "Reliability/Failover/ra/ProxyUpdateServiceDescriptionReplyMessageBody.h"
#include "Reliability/Failover/ra/ProxyQueryReplyMessageBody.h"
#include "Reliability/Failover/ra/ProxyMessageFlags.h"
#include "Reliability/Failover/ra/ProxyRequestMessageBody.h"
#include "Reliability/Failover/ra/ProxyReplyMessageBody.h"
#include "Reliability/Failover/ra/ReportFaultMessageBody.h"
#include "Reliability/Failover/ra/Message.h"
#include "Reliability/Failover/ra/ComProxyStatelessService.h"
#include "Reliability/Failover/ra/ComProxyStatefulService.h"
#include "Reliability/Failover/ra/ProxyId.h"
#include "Reliability/Failover/ra/FailoverUnitProxyContext.h"
#include "Reliability/Failover/ra/ReconfigurationAgentProxy.h"
#include "Reliability/Failover/ra/ComProxyReplicator.h"
#include "Reliability/Failover/ra/RAPTransport.h"
#include "Reliability/Failover/ra/ProxyUtility.h"
#include "Reliability/Failover/ra/FailoverUnitReconfigurationStage.h"
#include "Reliability/Failover/ra/FailoverUnitStates.h"
#include "Reliability/Failover/ra/ReplicaMessageStage.h"
#include "Reliability/Failover/ra/ProxyMessageStage.h"
#include "Reliability/Failover/ra/ReplicaStates.h"
#include "Reliability/Failover/ra/ExecutingOperation.h"
#include "Reliability/Failover/ra/Replica.h"
#include "Reliability/Failover/ra/ReplicaProxyStates.h"
#include "Reliability/Failover/ra/ReplicaProxy.h"
#include "Reliability/Failover/ra/GenerationState.h"
#include "Reliability/Failover/ra/FailoverUnitProxyStates.h"
#include "Reliability/Failover/ra/FailoverUnitProxyLifeCycleState.h"
#include "Reliability/Failover/ra/ReplicatorStates.h"
#include "Reliability/Failover/ra/ReadWriteStatusValue.h"
#include "Reliability/Failover/ra/ReadWriteStatusState.h"
#include "Reliability/Failover/ra/ComServicePartitionInfo.h"
#include "Reliability/Failover/ra/ComServicePartitionBase.h"
#include "Reliability/Failover/ra/ComStatelessServicePartition.h"
#include "Reliability/Failover/ra/ComStatefulServicePartition.h"
#include "Reliability/Failover/ra/LocalFailoverUnitProxyMap.h"
#include "Reliability/Failover/ra/LocalLoadReportingComponent.h"
#include "Reliability/Failover/ra/LocalMessageSenderComponent.h"
#include "Reliability/Failover/ra/LocalHealthReportingComponent.h"
#include "Reliability/Failover/ra/ExecutingOperationList.h"
#include "Reliability/Failover/ra/OperationManager.h"
#include "Reliability/Failover/ra/FailoverUnitProxyOperationManagerBase.h"
#include "Reliability/Failover/ra/ServiceOperationManager.h"
#include "Reliability/Failover/ra/ReplicatorOperationManager.h"
#include "Reliability/Failover/ra/ProxyActionsListTypes.h"
#include "Reliability/Failover/ra/ProxyConfigurationStage.h"
#include "Reliability/Failover/ra/LockedFailoverUnitProxyPtr.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ConfigurationUtility.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ReadWriteStatusCalculator.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.UserApiInvoker.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.UserApiInvokerAsyncOperationBase.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.CloseAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.OpenInstanceAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.CloseInstanceAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.OpenReplicaAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ChangeReplicaRoleAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.CloseReplicaAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.OpenReplicatorAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ChangeReplicatorRoleAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.CloseReplicatorAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ReplicatorBuildIdleReplicaAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ReplicatorCatchupReplicaSetAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ReplicatorOnDataLossAsyncOperation.h"
#include "Reliability/Failover/ra/FailoverUnitProxy.ReplicatorUpdateEpochAsyncOperation.h"
#include "Reliability/Failover/ra/ReconfigurationAgentProxy.ActionListExecutorAsyncOperation.h"
#include "Reliability/Failover/ra/ReconfigurationAgentProxy.ActionListExecutorReplicatorQueryAsyncOperation.h"
#include "Reliability/Failover/ra/ReconfigurationAgentProxy.CloseAsyncOperation.h"
#include "Reliability/Failover/ra/ProxyOutgoingMessage.h"
#include "Reliability/Failover/ra/ProxyEventSource.h"

// API headers
#include "Reliability/Failover/ra/Storage.Api.RowType.h"
#include "Reliability/Failover/ra/Storage.Api.OperationType.h"
#include "Reliability/Failover/ra/Storage.Api.RowIdentifier.h"
#include "Reliability/Failover/ra/Storage.Api.RowData.h"
#include "Reliability/Failover/ra/Storage.Api.Row.h"
#include "Reliability/Failover/ra/Storage.Api.IKeyValueStore.h"

// Infrastructure headers
#include "Reliability/Failover/ra/Infrastructure.IClock.h"
#include "Reliability/Failover/ra/Infrastructure.RAStopwatch.h"
#include "Reliability/Failover/ra/Infrastructure.EntitySetName.h"
#include "Reliability/Failover/ra/Diagnostics.ReconfigurationPerformanceData.h"
#include "Reliability/Failover/ra/Diagnostics.PerformanceCounters.h"
#include "Reliability/Failover/ra/Diagnostics.FMMessageRetryPerformanceData.h"
#include "Reliability/Failover/ra/Diagnostics.ScheduleEntityPerformanceData.h"
#include "Reliability/Failover/ra/Diagnostics.ExecuteEntityJobItemListPerformanceData.h"
#include "Reliability/Failover/ra/Diagnostics.CommitEntityPerformanceData.h"
#include "Reliability/Failover/ra/Diagnostics.FilterEntityMapPerformanceData.h"
#include "Reliability/Failover/ra/Infrastructure.FilteredVector.h"
#include "Reliability/Failover/ra/Infrastructure.IThrottle.h"
#include "Reliability/Failover/ra/Infrastructure.IRetryPolicy.h"
#include "Reliability/Failover/ra/Infrastructure.LinearRetryPolicy.h"
#include "Reliability/Failover/ra/Infrastructure.RetryState.h"
#include "Reliability/Failover/ra/Infrastructure.EntityRetryComponent.h"

// Scheduling and execution of work
#include "Reliability/Failover/ra/Infrastructure.AsyncJobQueue.h"
#include "Reliability/Failover/ra/Infrastructure.Threadpool.h"

// Infrastructure::JobItem enumerations and base definitions
#include "Reliability/Failover/ra/Infrastructure.EntityJobItemList.h"
#include "Reliability/Failover/ra/Infrastructure.EntityTraits.h"
#include "Reliability/Failover/ra/Infrastructure.EntityEntryBase.h"
#include "Reliability/Failover/ra/Infrastructure.IEntityStateBase.h"

// Infrastructure::JobItem related
#include "Reliability/Failover/ra/Infrastructure.UpdateContext.h"
#include "Reliability/Failover/ra/Infrastructure.StateUpdateContext.h"
#include "Reliability/Failover/ra/Infrastructure.EntityAssertContext.h"
#include "Reliability/Failover/ra/Infrastructure.EntityExecutionContext.h"
#include "Reliability/Failover/ra/Infrastructure.JobItemName.h"
#include "Reliability/Failover/ra/Infrastructure.JobItemCheck.h"
#include "Reliability/Failover/ra/Infrastructure.JobItemTraceFrequency.h"
#include "Reliability/Failover/ra/Infrastructure.JobItemDescription.h"
#include "Reliability/Failover/ra/Infrastructure.EntityJobItemTraceInformation.h"
#include "Reliability/Failover/ra/Infrastructure.EntityJobItemBase.h"
#include "Reliability/Failover/ra/Infrastructure.MultipleEntityWork.h"

// Persistence
#include "Reliability/Failover/ra/Infrastructure.LockType.h"
#include "Reliability/Failover/ra/Infrastructure.EntityStateSnapshot.h"
#include "Reliability/Failover/ra/Infrastructure.EntityState.h"
#include "Reliability/Failover/ra/Infrastructure.EntityScheduler.h"
#include "Reliability/Failover/ra/Infrastructure.LockedEntityPtrBase.h"
#include "Reliability/Failover/ra/Infrastructure.ReadOnlyLockedEntityPtr.h"
#include "Reliability/Failover/ra/Infrastructure.CommitTypeDescription.h"
#include "Reliability/Failover/ra/Infrastructure.CommitDescription.h"
#include "Reliability/Failover/ra/Infrastructure.EntityPreCommitNotificationSink.h"
#include "Reliability/Failover/ra/Infrastructure.EntityMap.h"
#include "Reliability/Failover/ra/Infrastructure.LockedEntityPtr.h"
#include "Reliability/Failover/ra/Infrastructure.EntityEntry.h"
#include "Reliability/Failover/ra/Infrastructure.EntityMapAlgorithm.h"
#include "Reliability/Failover/ra/Infrastructure.EntityJobItemBaseT.h"
#include "Reliability/Failover/ra/Infrastructure.EntityJobItemListTracer.h"
#include "Reliability/Failover/ra/Infrastructure.EntityJobItemListExecutorAsyncOperation.h"
#include "Reliability/Failover/ra/Infrastructure.ScheduleAndExecuteEntityJobItemAsyncOperation.h"

// StateMachine Action
#include "Reliability/Failover/ra/Infrastructure.StateMachineAction.h"
#include "Reliability/Failover/ra/Infrastructure.StateMachineActionQueue.h"
#include "Reliability/Failover/ra/Infrastructure.SendMessageActions.h"

// Entity Job Item
#include "Reliability/Failover/ra/Infrastructure.HandlerParameters.h"
#include "Reliability/Failover/ra/Infrastructure.EntityJobItem.h"

// Failover Unit Sets
#include "Reliability/Failover/ra/Infrastructure.EntitySetIdentifier.h"
#include "Reliability/Failover/ra/Infrastructure.EntitySetCollection.h"
#include "Reliability/Failover/ra/Infrastructure.SetMembershipFlag.h"
#include "Reliability/Failover/ra/Infrastructure.EntitySet.h"

// Utility classes 
#include "Reliability/Failover/ra/Infrastructure.Clock.h"
#include "Reliability/Failover/ra/Infrastructure.BackgroundWorkRetryType.h"
#include "Reliability/Failover/ra/Infrastructure.RetryTimer.h"
#include "Reliability/Failover/ra/Infrastructure.BackgroundWorkManager.h"
#include "Reliability/Failover/ra/Infrastructure.BackgroundWorkManagerWithRetry.h"
#include "Reliability/Failover/ra/Infrastructure.ExpiringMap.h"
#include "Reliability/Failover/ra/Infrastructure.MultipleEntityBackgroundWorkManager.h"
#include "Reliability/Failover/ra/Infrastructure.MultipleEntityWorkManager.h"

#include "Reliability/Failover/ra/Infrastructure.MessageUtility.h"

#include "Reliability/Failover/ra/Infrastructure.JobQueueManager.h"

// Utility
#include "Reliability/Failover/ra/Utility.SystemServiceHelper.h"

// Storage
#include "Reliability/Failover/ra/Storage.InMemoryKeyValueStoreState.h"
#include "Reliability/Failover/ra/Storage.InMemoryKeyValueStore.h"
#include "Reliability/Failover/ra/Storage.LocalStoreAdapter.h"
#include "Reliability/Failover/ra/Storage.FaultInjectionAdapter.h"
#include "Reliability/Failover/ra/Storage.KeyValueStoreFactory.h"

// Communication
#include "Reliability/Failover/ra/Communication.MessageTarget.h"
#include "Reliability/Failover/ra/Communication.StalenessCheckType.h"
#include "Reliability/Failover/ra/Communication.MessageMetadata.h"
#include "Reliability/Failover/ra/Communication.MessageMetadataTable.h"

// Hosting
#include "Reliability/Failover/ra/Hosting.FindServiceTypeRegistrationErrorCode.h"
#include "Reliability/Failover/ra/Hosting.FindServiceTypeRegistrationErrorList.h"
#include "Reliability/Failover/ra/Hosting.HostingEventName.h"
#include "Reliability/Failover/ra/Hosting.TerminateServiceHostReason.h"
#include "Reliability/Failover/ra/Hosting.TerminateHostStateMachineAction.h"
#include "Reliability/Failover/ra/Hosting.ServiceTypeRegistrationChangeStateMachineAction.h"
#include "Reliability/Failover/ra/Hosting.ServiceTypeMapEntity.h"
#include "Reliability/Failover/ra/Hosting.ServiceTypeMap.h"
#include "Reliability/Failover/ra/Hosting.HostingEventHandler.h"
#include "Reliability/Failover/ra/Hosting.HostingAdapter.h"

// Health
#include "Reliability/Failover/ra/Health.ReplicaHealthEvent.h"
#include "Reliability/Failover/ra/Health.HealthContext.h"
#include "Reliability/Failover/ra/Health.HealthReportDescriptorBase.h"
#include "Reliability/Failover/ra/Health.IHealthSubsystemWrapper.h"
#include "Reliability/Failover/ra/Health.HealthSubsystemWrapper.h"
#include "Reliability/Failover/ra/Health.ReportReplicaHealthStateMachineAction.h"


// Communication
#include "Reliability/Failover/ra/Communication.FMMessageData.h"
#include "Reliability/Failover/ra/Communication.FMMessageDescription.h"
#include "Reliability/Failover/ra/Communication.FMMessageBuilder.h"
#include "Reliability/Failover/ra/Communication.FMTransport.h"
#include "Reliability/Failover/ra/Communication.RequestFMMessageRetryAction.h"

// Node
#include "Reliability/Failover/ra/Node.NodeDeactivationInfo.h"
#include "Reliability/Failover/ra/Node.NodeDeactivationState.h"
#include "Reliability/Failover/ra/Node.NodeDeactivationStateProcessor.h"
#include "Reliability/Failover/ra/Node.PendingReplicaUploadState.h"
#include "Reliability/Failover/ra/Node.PendingReplicaUploadStateProcessor.h"
#include "Reliability/Failover/ra/Node.FMMessageThrottle.h"
#include "Reliability/Failover/ra/Node.NodeState.h"
#include "Reliability/Failover/ra/Node.NodeDeactivationMessageProcessor.h"

// FT State Machine

#include "Reliability/Failover/ra/HealthReportReplica.h"
#include "Reliability/Failover/ra/PartitionHealthEventDescriptor.h"
#include "Reliability/Failover/ra/ReconfigurationProgressStages.h"
#include "Reliability/Failover/ra/ReconfigurationStuckHealthReportDescriptor.h"
#include "Reliability/Failover/ra/ClearWarningErrorHealthReportDescriptor.h"
#include "Reliability/Failover/ra/ReconfigurationHealthState.h"
#include "Reliability/Failover/ra/TransitionErrorCode.h"
#include "Reliability/Failover/ra/TransitionHealthReportDescriptor.h"
#include "Reliability/Failover/ra/RAReplicaOpenMode.h"
#include "Reliability/Failover/ra/ReplicaCloseModeName.h"
#include "Reliability/Failover/ra/ReplicaCloseMode.h"
#include "Reliability/Failover/ra/RetryableErrorStateName.h"
#include "Reliability/Failover/ra/Node.ServiceTypeUpdateStalenessChecker.h"
#include "Reliability/Failover/ra/Node.ServiceTypeUpdatePendingLists.h"
#include "Reliability/Failover/ra/Node.ServiceTypeUpdateProcessor.h"
#include "Reliability/Failover/ra/Node.ServiceTypeUpdateResult.h"
#include "Reliability/Failover/ra/RetryableErrorStateThresholdCollection.h"
#include "Reliability/Failover/ra/ReplicaUploadType.h"
#include "Reliability/Failover/ra/RetryableErrorAction.h"
#include "Reliability/Failover/ra/RetryableErrorState.h"
#include "Reliability/Failover/ra/ReconfigurationState.h"
#include "Reliability/Failover/ra/FMMessageState.h"

#include "Reliability/Failover/ra/ReplicaUploadState.h"
#include "Reliability/Failover/ra/ServiceTypeRegistrationWrapper.h"
#include "Reliability/Failover/ra/FailoverUnitTransportUtility.h"
#include "Reliability/Failover/ra/ReplicaStore.h"
#include "Reliability/Failover/ra/EndpointPublishState.h"
#include "Reliability/Failover/ra/ReconfigurationStuckHealthReportDescriptorFactory.h"

#include "Reliability/Failover/ra/FailoverUnitEntityExecutionContext.h"
#include "Reliability/Failover/ra/FailoverUnit.h"
#include "Reliability/Failover/ra/FailoverUnitPreCommitValidator.h"
#include "Reliability/Failover/ra/Infrastructure.FailoverUnitEntityTraits.h"
#include "Reliability/Failover/ra/Infrastructure.FailoverUnitJobItemHandlerParameters.h"

#include "Reliability/Failover/ra/JobItemContext.h"
#include "Reliability/Failover/ra/MessageContext.h"
#include "Reliability/Failover/ra/MultipleReplicaCloseCompletionCheckAsyncOperation.h"
#include "Reliability/Failover/ra/MultipleReplicaCloseAsyncOperation.h"

// Upgrade
#include "Reliability/Failover/ra/Upgrade.FabricUpgradeStalenessCheckResult.h"
#include "Reliability/Failover/ra/Upgrade.FabricUpgradeStalenessChecker.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeStateCancelBehaviorType.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeCancelMode.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeCancelResult.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeStateName.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeStateDescription.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeRollbackSnapshot.h"
#include "Reliability/Failover/ra/Upgrade.CancelUpgradeContext.h"
#include "Reliability/Failover/ra/Upgrade.Upgrade.h"
#include "Reliability/Failover/ra/Upgrade.FabricUpgrade.h"
#include "Reliability/Failover/ra/Upgrade.ApplicationUpgrade.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeStateMachine.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeMessageProcessor.h"
#include "Reliability/Failover/ra/Upgrade.FabricCodeVersionClassifier.h"

// Message Retry
#include "Reliability/Failover/ra/MessageRetry.FMMessagePendingEntityList.h"
#include "Reliability/Failover/ra/MessageRetry.FMMessageSender.h"
#include "Reliability/Failover/ra/MessageRetry.FMMessageRetryComponent.h"

#include "Reliability/Failover/ra/GenerationStateManager.h"
#include "Reliability/Failover/ra/MessageHandler.h"
#include "Reliability/Failover/ra/ReconfigurationAgent.h"

#include "Reliability/Failover/ra/Infrastructure.LocalFailoverUnitMap.h"
#include "Reliability/Failover/ra/Infrastructure.FailoverUnitJobItem.h"

#include "Reliability/Failover/ra/MessageBodyTypeTraits.h"
#include "Reliability/Failover/ra/NodeUpAckProcessor.h"
#include "Reliability/Failover/ra/LocalStorageInitializer.h"

//query
#include "Reliability/Failover/ra/Query.IQueryHandler.h"
#include "Reliability/Failover/ra/Query.DeployedServiceReplicaUtility.h"
#include "Reliability/Failover/ra/Query.QueryUtility.h"
#include "Reliability/Failover/ra/Query.HostQueryAsyncOperation.h"
#include "Reliability/Failover/ra/Query.ReplicaDetailQueryHandler.h"
#include "Reliability/Failover/ra/Query.ReplicaListQueryHandler.h"
#include "Reliability/Failover/ra/Query.QueryHelper.h"

// These infrastructure headers need to be at the end as they need structs and classes
// from other headers to be visible for their definition
#include "Reliability/Failover/ra/Infrastructure.EventSource.h"

#include "Reliability/Failover/ra/ComProxyReplicator.OpenAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyReplicator.BuildIdleReplicaAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyReplicator.CatchupReplicaSetAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyReplicator.ChangeRoleAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyReplicator.CloseAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyReplicator.OnDataLossAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyReplicator.UpdateEpochAsyncOperation.h"

#include "Reliability/Failover/ra/ComProxyStatelessService.OpenAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyStatelessService.CloseAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyStatefulService.OpenAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyStatefulService.ChangeRoleAsyncOperation.h"
#include "Reliability/Failover/ra/ComProxyStatefulService.CloseAsyncOperation.h"

#include "Reliability/Failover/TestApi.FailoverPointers.h"
#include "Reliability/Failover/ra/TestApi.RA.h"
#include "Reliability/Failover/ra/TestApi.RAP.h"

extern const std::wstring DefaultActivityId;
