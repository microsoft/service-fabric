// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <sal.h>
#include <esent.h>

#include "Common/Common.h"

#include "Transport/Transport.h"

#include "Federation/Federation.h"

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "Common/RetailSerialization.h"

//
// Replication header files
//


// Forward declaration of well-known pointer types and associated classes/structs
#include "Reliability/Replication/Replication.Public.h"

#include "Reliability/Replication/Constants.h"
#include "Reliability/Replication/reconfig.h"
#include "Reliability/Replication/REInternalSettings.h"
#include "Reliability/Replication/IOperationDataFactory.h"
#include "Reliability/Replication/ComReplicator.h"
#include "Reliability/Replication/ComReplicatorFactory.h"

#include "Reliability/Replication/HealthReportType.h"
#include "Reliability/Replication/MustCatchupEnum.h"
#include "Reliability/Replication/ReplicationEndpointId.h"
#include "Reliability/Replication/ComProxyStatefulServicePartition.h"
#include "Reliability/Replication/ComProxyStateProvider.h"
#include "Reliability/Replication/ReplicatorState.h"
#include "Reliability/Replication/DecayAverage.h"
#include "Reliability/Replication/ReliableOperationSender.h"
#include "Reliability/Replication/CopySender.h"
#include "Reliability/Replication/OperationQueue.h"
#include "Reliability/Replication/ReplicationQueueManager.h"
#include "Reliability/Replication/standarddeviation.h"
#include "Reliability/Replication/RemoteSession.h"
#include "Reliability/Replication/ReplicationSession.h"
#include "Reliability/Replication/ReplicaManager.h"
#include "Reliability/Replication/Replicator.h"

#include "Reliability/Replication/SecondaryReplicatorTraceInfo.h"
#include "Reliability/Replication/OperationQueueEventSource.h"
#include "Reliability/Replication/ReplicatorEventSource.h"
