// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This header contains includes that are private to Failover
#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"

#include "Reliability/Failover/ra/ra.public.h"
#include "Reliability/Failover/fm/fm.public.h"

#include "Federation/Federation.h"
#include "Transport/Transport.h"

#include "Hosting2/Hosting.Public.h"
#include "Hosting2/Hosting.Runtime.Public.h"

#include "Reliability/LoadBalancing/PlacementAndLoadBalancingPublic.h"

#include "Store/store.h"

#include "Reliability/Replication/ReplicationPointers.h"
#include "data/txnreplicator/TransactionalReplicatorPointers.h"

#include "client/ClientConfig.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

#include "Reliability/Failover/FailoverPointers.h"

#include "Reliability/Failover/IFederationWrapper.h"
#include "Reliability/Failover/IReliabilitySubsystem.h"

#include "Reliability/Failover/common/Common.Internal.h"

#include "Reliability/Failover/ChangeNotificationOperation.h"

#include "Reliability/Failover/FederationWrapper.h"

#include "Reliability/Failover/CachedServiceTableEntry.h"
#include "Reliability/Failover/MatchedServiceTableEntry.h"
#include "Reliability/Failover/IClientRegistration.h"
#include "Reliability/Failover/IServiceTableEntryCache.h"
#include "Reliability/Failover/NotificationCacheIndex.h"
#include "Reliability/Failover/BroadcastProcessedEventArgs.h"
#include "Reliability/Failover/CacheMode.h"
#include "Reliability/Failover/LookupVersionStoreProvider.h"
#include "Reliability/Failover/ServiceResolver.h"
#include "Reliability/Failover/ServiceResolverCache.h"
#include "Reliability/Failover/ServiceResolverImpl.h"

#include "Reliability/Failover/ServiceAdminClient.h"
#include "Reliability/Failover/fm/ServiceLookupTable.h"

#include "Reliability/Failover/ReliabilitySubsystem.h"
#include "Reliability/Failover/ra/IReliabilitySubsystemWrapper.h"
#include "Reliability/Failover/ReliabilitySubsystemWrapper.h"
