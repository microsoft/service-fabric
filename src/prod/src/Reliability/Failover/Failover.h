// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <sal.h>
#include <esent.h>

#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"

#include "Store/store.h"

#include "Federation/Federation.h"
#include "query/Query.h"
#include "Hosting2/Hosting.Public.h"
#include "Hosting2/Hosting.Runtime.Public.h"
#include "Reliability/Replication/Replication.Public.h"
#include "data/txnreplicator/TransactionalReplicator.Public.h"
#include "client/client.h"

#include "FabricClient.h"

// Forward declaration of well-known pointer types and associated classes/structs
#include "Reliability/Failover/FailoverPointers.h"

// Common
#include "Management/NetworkInventoryManager/common/NIMMessage.h"
#include "Reliability/Failover/common/VersionedCuid.h"
#include "Reliability/Failover/common/UpgradeDescription.h"
#include "Reliability/Failover/common/ReplicaStatus.h"
#include "Reliability/Failover/common/RSMessage.h"
#include "Reliability/Failover/common/FailoverConfig.h"

// FM
#include "Reliability/Failover/fm/IFailoverManager.h"
#include "Reliability/Failover/fm/RecoverPartitionsRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverPartitionRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverServicePartitionsRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverSystemPartitionsRequestMessageBody.h"

#include "Reliability/Failover/fm/BasicFailoverRequestMessageBody.h"
#include "Reliability/Failover/fm/BasicFailoverReplyMessageBody.h"

#include "Reliability/Failover/fm/ServiceTableRequestMessageBody.h"
#include "Reliability/Failover/fm/ServiceTableUpdateMessageBody.h"

#include "Reliability/Failover/fm/CreateServiceMessageBody.h"
#include "Reliability/Failover/fm/CreateServiceReplyMessageBody.h"
#include "Reliability/Failover/fm/UpdateServiceMessageBody.h"
#include "Reliability/Failover/fm/UpdateSystemServiceMessageBody.h"
#include "Reliability/Failover/fm/UpdateServiceReplyMessageBody.h"
#include "Reliability/Failover/fm/DeleteApplicationRequestMessageBody.h"
#include "Reliability/Failover/fm/DeleteApplicationReplyMessageBody.h"
#include "Reliability/Failover/fm/DeleteServiceMessageBody.h"
#include "Reliability/Failover/fm/ServiceDescriptionReplyMessageBody.h"

#include "Reliability/Failover/fm/UpgradeRequestMessageBody.h"
#include "Reliability/Failover/fm/UpgradeReplyMessageBody.h"
#include "Reliability/Failover/fm/UpgradeFabricReplyMessageBody.h"
#include "Reliability/Failover/fm/UpgradeApplicationReplyMessageBody.h"
#include "Reliability/Failover/fm/ServiceTableUpdateMessageBody.h"
#include "Reliability/Failover/fm/ServiceTableRequestMessageBody.h"

#include "Reliability/Failover/fm/ActivateNodeRequestMessageBody.h"
#include "Reliability/Failover/fm/NodeStateRemovedRequestMessageBody.h"

#include "Reliability/Failover/fm/CreateApplicationRequestMessageBody.h"
#include "Reliability/Failover/fm/UpdateApplicationRequestMessageBody.h"


// RA
#include "Reliability/Failover/ra/IReconfigurationAgentProxy.h"

// Reliability
#include "Reliability/Failover/CachedServiceTableEntry.h"
#include "Reliability/Failover/MatchedServiceTableEntry.h"
#include "Reliability/Failover/IClientRegistration.h"
#include "Reliability/Failover/IServiceTableEntryCache.h"
#include "Reliability/Failover/NotificationCacheIndex.h"
#include "Reliability/Failover/CacheMode.h"
#include "Reliability/Failover/BroadcastProcessedEventArgs.h"
#include "Reliability/Failover/IService.h"
#include "Reliability/Failover/IFederationWrapper.h"
#include "Reliability/Failover/FederationWrapper.h"
#include "Reliability/Failover/ServiceAdminClient.h"
#include "Reliability/Failover/ServiceResolver.h"
#include "Reliability/Failover/FederationWrapper.h"
#include "Reliability/Failover/IReliabilitySubsystem.h"

