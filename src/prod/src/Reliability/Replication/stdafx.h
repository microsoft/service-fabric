// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// System headers
//

#include <sal.h>
#include <esent.h>

//
// WinFab project dependencies
//

#include "api/wrappers/ApiWrappers.h"

#include "Transport/Transport.h"
#include "Transport/UnreliableTransport.h"
#include "Transport/Throttle.h"

//
// Replication header files
//
#include "FabricRuntime_.h"


// Public replication header
#include "Reliability/Replication/Replication.h"

// N.B. Remainder is files not in Replication.h

// Replication

#include "Reliability/Replication/ReplicationActorHeader.h"
#include "Reliability/Replication/ReplicationDemuxer.h"
#include "Reliability/Replication/ReplicaInformation.h"
#include "Reliability/Replication/ComProxyAsyncEnumOperation.h"
#include "Reliability/Replication/ComOperationDataAsyncEnumerator.h"
#include "Reliability/Replication/ComOperation.h"
#include "Reliability/Replication/ComUserDataOperation.h"
#include "Reliability/Replication/ComFromBytesOperation.h"
#include "Reliability/Replication/ComUpdateEpochOperation.h"
#include "Reliability/Replication/ComStartCopyOperation.h"
#include "Reliability/Replication/ComOperationData.h"
#include "Reliability/Replication/EnumOperators.h"
#include "Reliability/Replication/StartCopyMessageBody.h"
#include "Reliability/Replication/InduceFaultMessageBody.h"
#include "Reliability/Replication/AckSender.h"
#include "Reliability/Replication/CopyAsyncOperation.h"
#include "Reliability/Replication/CopyContextReceiver.h"
#include "Reliability/Replication/RemoteSession.EstablishCopyAsyncOperation.h"
#include "Reliability/Replication/ReplicationAckMessageBody.h"
#include "Reliability/Replication/ReplicationFromHeader.h"
#include "Reliability/Replication/ReplicationOperationHeader.h"
#include "Reliability/Replication/ReplicationOperationBodyHeader.h"
#include "Reliability/Replication/CopyOperationHeader.h"
#include "Reliability/Replication/CopyContextOperationHeader.h"
#include "Reliability/Replication/OperationAckHeader.h"
#include "Reliability/Replication/AckMessageBody.h"
#include "Reliability/Replication/OperationErrorHeader.h"
#include "Reliability/Replication/ReplicationTransport.h"
#include "Reliability/Replication/PrimaryReplicator.h"
#include "Reliability/Replication/PrimaryReplicator.BuildIdleAsyncOperation.h"
#include "Reliability/Replication/PrimaryReplicator.CloseAsyncOperation.h"
#include "Reliability/Replication/PrimaryReplicator.ReplicateAsyncOperation.h"
#include "Reliability/Replication/PrimaryReplicator.CatchupAsyncOperation.h"
#include "Reliability/Replication/PrimaryReplicator.UpdateEpochAsyncOperation.h"
#include "Reliability/Replication/OperationStream.h"
#include "Reliability/Replication/OperationStream.GetOperationAsyncOperation.h"
#include "Reliability/Replication/SecondaryCopyReceiver.h"
#include "Reliability/Replication/SecondaryReplicationReceiver.h"
#include "Reliability/Replication/SecondaryReplicatorTraceInfo.h"
#include "Reliability/Replication/SecondaryReplicatorTraceHandler.h"
#include "Reliability/Replication/SecondaryReplicator.h"
#include "Reliability/Replication/SecondaryReplicator.CloseAsyncOperation.h"
#include "Reliability/Replication/SecondaryReplicator.CloseAsyncOperation2.h"
#include "Reliability/Replication/SecondaryReplicator.UpdateEpochAsyncOperation.h"
#include "Reliability/Replication/SecondaryReplicator.DrainQueueAsyncOperation.h"
#include "Reliability/Replication/Replicator.h"
#include "Reliability/Replication/Replicator.BuildIdleAsyncOperation.h"
#include "Reliability/Replication/Replicator.CloseAsyncOperation.h"
#include "Reliability/Replication/Replicator.ChangeRoleAsyncOperation.h"
#include "Reliability/Replication/Replicator.CatchupReplicaSetAsyncOperation.h"
#include "Reliability/Replication/Replicator.OpenAsyncOperation.h"
#include "Reliability/Replication/Replicator.ReplicateAsyncOperation.h"
#include "Reliability/Replication/Replicator.OnDatalossAsyncOperation.h"
#include "Reliability/Replication/Replicator.UpdateEpochAsyncOperation.h"
#include "Reliability/Replication/REPerformanceCounters.h"
#include "Reliability/Replication/ComOperationStream.h"
#include "Reliability/Replication/ComOperationStream.GetOperationOperation.h"
#include "Reliability/Replication/ComReplicator.BaseOperation.h"
#include "Reliability/Replication/ComReplicator.BuildIdleReplicaOperation.h"
#include "Reliability/Replication/ComReplicator.ChangeRoleOperation.h"
#include "Reliability/Replication/ComReplicator.CloseOperation.h"
#include "Reliability/Replication/ComReplicator.OpenOperation.h"
#include "Reliability/Replication/ComReplicator.ReplicateOperation.h"
#include "Reliability/Replication/ComReplicator.CatchupReplicaSetOperation.h"
#include "Reliability/Replication/ComReplicator.OnDatalossOperation.h"
#include "Reliability/Replication/ComReplicator.UpdateEpochOperation.h"
#include "Reliability/Replication/ComReplicatorFactory.h"
#include "Reliability/Replication/BatchedHealthReporter.h"
#include "Reliability/Replication/ApiMonitoringWrapper.h"
