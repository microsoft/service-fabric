// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// External headers needed by the fabric client
//

#if !defined(PLATFORM_UNIX)
#include <IntSafe.h>
#endif

#include "Common/Common.h"
#include "Federation/Federation.h"
#include "systemservices/SystemServices.Messages.h"
#include "ServiceModel/ServiceModel.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"
#include "Management/ImageStore/IImageStore.h"
#include "Management/ImageStore/ImageStoreFactory.h"
#include "Management/Common/ManagementConfig.h"
#include "Management/RepairManager/RepairManager.External.h"

//
// Fabric client internal headers
//

#include "client/ClientConfig.h"

#include "client/constants.h"
#include "client/Utility.h"
#include "client/ApplicationUpgradeProgressResult.h"
#include "client/FabricClientInternalSettings.h"
#include "client/INotificationClientSettings.h"
#include "client/FabricClientInternalSettingsHolder.h"

// Health Reporting Component
#include "client/HealthReportClientState.h"
#include "client/HealthReportWrapper.h"
#include "client/HealthReportSortedList.h"
#include "client/HealthReportingTransport.h"
#include "client/HealthReportingComponent.h"
#include "client/HealthReportingComponent.SequenceStream.h"
#include "client/HealthClientEventSource.h"
#include "client/IpcHealthReportingClient.h"

#include "client/OrchestrationUpgradeStatusResult.h"
#include "client/UpgradeOrchestrationServiceStateResult.h"
#include "client/PropertyBatchResult.h"
#include "client/UpgradeProgressResult.h"
#include "client/InvokeDataLossProgressResult.h"
#include "client/InvokeQuorumLossProgressResult.h"
#include "client/RestartPartitionProgressResult.h"
#include "client/NodeTransitionProgressResult.h"
#include "client/ChaosReportResult.h"
#include "client/ChaosEventsSegmentResult.h"
#include "client/CallSystemServiceResult.h"
#include "client/ClientHealthReporting.h"
#include "client/ResolvedServicePartitionCacheEntry.h"
#include "client/LruClientCacheCallback.h"
#include "client/LruClientCacheEntry.h"
#include "client/IClientCache.h"
#include "client/ResolvedServicePartitionResult.h"
#include "client/ServiceAddressTrackerPollRequests.h"
#include "client/ServiceAddressTrackerCallbacks.h"
#include "client/ServiceAddressTracker.h"
#include "client/ServiceAddressTrackerManager.h"
#include "client/ClientEventSource.h"
#include "client/ChaosDescriptionResult.h"
#include "client/ChaosScheduleDescriptionResult.h"

//
// Service Notifications
//

#include "client/ServiceNotificationResult.h"
#include "client/INotificationClientCache.h"
#include "client/ClientConnectionManager.h"
#include "client/pendingServiceNotificationResultsItem.h"
#include "client/ServiceNotificationClient.h"
#include "client/LruClientCacheManager.h"
#include "client/LruClientCacheManager.CacheAsyncOperationBase.h"
#include "client/LruClientCacheManager.ResolveServiceAsyncOperation.h"
#include "client/LruClientCacheManager.UpdateCacheEntryAsyncOperation.h"
#include "client/LruClientCacheManager.ParallelUpdateCacheEntriesAsyncOperation.h"
#include "client/LruClientCacheManager.UpdateServiceTableEntryAsyncOperation.h"
#include "client/LruClientCacheManager.ProcessServiceNotificationAsyncOperation.h"

#include "client/PropertyBatchResult.h"
#include "client/UpgradeProgressResult.h"
#include "client/ClientHealthReporting.h"
#include "client/ResolvedServicePartitionCacheEntry.h"
#include "client/LruClientCacheCallback.h"
#include "client/LruClientCacheEntry.h"
#include "client/IClientCache.h"
#include "client/ResolvedServicePartitionResult.h"
#include "client/ServiceAddressTrackerPollRequests.h"
#include "client/ServiceAddressTrackerCallbacks.h"
#include "client/ServiceAddressTracker.h"
#include "client/ServiceAddressTrackerManager.h"
#include "client/LruClientCacheManager.h"
#include "client/LruClientCacheManager.CacheAsyncOperationBase.h"
#include "client/LruClientCacheManager.GetPsdAsyncOperation.h"
#include "client/LruClientCacheManager.ResolveServiceAsyncOperation.h"
#include "client/LruClientCacheManager.UpdateCacheEntryAsyncOperation.h"
#include "client/LruClientCacheManager.ParallelUpdateCacheEntriesAsyncOperation.h"
#include "client/LruClientCacheManager.UpdateServiceTableEntryAsyncOperation.h"
#include "client/LruClientCacheManager.PrefixResolveServiceAsyncOperation.h"
#include "client/LruClientCacheManager.ProcessServiceNotificationAsyncOperation.h"
#include "client/RestartNodeResult.h"
#include "client/StartNodeResult.h"
#include "client/StopNodeResult.h"
#include "client/RestartDeployedCodePackageResult.h"
#include "client/MovePrimaryResult.h"
#include "client/MoveSecondaryResult.h"

#include "client/FileSender.h"
#include "client/FileReceiver.h"
#include "client/FileTransferClient.h"
#include "client/FabricClientImpl.h"
#include "client/FabricClientImpl.ClientAsyncOperationBase.h"
#include "client/FabricClientImpl.ForwardToServiceAsyncOperation.h"
#include "client/FabricClientImpl.RequestReplyAsyncOperation.h"
#include "client/FabricClientImpl.GetServiceDescriptionAsyncOperation.h"
#include "client/FabricClientImpl.DeleteServiceAsyncOperation.h"
#include "client/FabricClientImpl.LocationChangeNotificationAsyncOperation.h"
#include "client/FabricClientImpl.CreateContainerAppAsyncOperation.h"
