// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{

#define BEGIN_REPLSTORE_STRUCTURED_TRACES \
    public: \
    ReplicatedStoreEventSource() :

#define END_REPLSTORE_STRUCTURED_TRACES \
    dummyMember() \
    { } \
    bool dummyMember; \

#define DECLARE_STATE_MACHINE_STRUCTURED_TRACE(trace_name, ...) \
    DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define STATE_MACHINE_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, ReplicatedStoreStateMachine, trace_id, trace_level, __VA_ARGS__),

#define DECLARE_REPLICATED_STORE_STRUCTURED_TRACE(trace_name, ...) \
    DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define REPLICATED_STORE_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, ReplicatedStore, trace_id, trace_level, __VA_ARGS__),    

    class ReplicatedStoreEventSource
    {
        BEGIN_REPLSTORE_STRUCTURED_TRACES

        STATE_MACHINE_STRUCTURED_TRACE( QueueEvent, 20, Noise, "{0} {1}", "prId", "smEvent" ) 
        STATE_MACHINE_STRUCTURED_TRACE( ProcessEvent, 30, Noise, "{0} {1}", "prId", "smEvent" ) 
        STATE_MACHINE_STRUCTURED_TRACE( StateChangeInfo, 40, Info, "{0} {1} -> {2}", "prId", "old", "new" ) 
        STATE_MACHINE_STRUCTURED_TRACE( StateChangeNoise, 50, Noise, "{0} {1} -> {2}", "prId", "old", "new" ) 
        STATE_MACHINE_STRUCTURED_TRACE( IncrementTransaction, 60, Noise, "{0} +{1}", "prId", "count" )
        STATE_MACHINE_STRUCTURED_TRACE( DecrementTransaction, 70, Noise, "{0} -{1}", "prId", "count" )

        REPLICATED_STORE_STRUCTURED_TRACE( OperationComplete, 80, Noise, "{0} operation LSN = '{1}' completed. Completer = {2}", "ractId", "operationLsn", "completerLsn")

        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryDummyCommit, 100, Noise, "{0} disk flush: last LSN = {1} last sync LSN = {2}.", "prId", "lastLsn", "lastSyncLsn")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpOperation, 101, Noise, "{0}", "prId")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpedCopy, 102, Noise, "{0} LSN={1} count={2} bytes={3} sync={4} type={5}",  "prId", "lsn", "count", "bytes", "sync", "copytype")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpedReplication, 103, Noise, "{0} repl = {1} acked = {2} count = {3} bytes = {4} sync = {5} activity = {6}", "prId", "lsn", "acked", "count", "bytes", "sync", "actId")

        // Note that some field names (e.g. "id", "type") are reserved special keywords for the structured tracing infrastructure
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryApply, 104, Noise, "{0} [{1}] type = '{2}' key = '{3}' data/repl = {4}/{5}", "prId", "op", "ty", "key", "opLsn", "replLsn")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryCommitFast, 105, Noise, "{0} LSN = {1} cost = {2} ms", "prId", "lsn", "cost")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryCommitSlow, 106, Info, "{0} LSN = {1} cost = {2} ms", "prId", "lsn", "cost")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpCtor, 107, Noise, "this = {0}", "this")
        REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpDtor, 108, Noise, "this = {0}", "this")

        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryComReplAsyncCtor, 110, Noise, "this = {0}", "this")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryComReplAsyncDtor, 111, Noise, "this = {0}", "this")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplAsyncCtor, 112, Noise, "{0} this = {1}", "ractId", "this")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplAsyncDtor, 113, Noise, "{0} this = {1}", "ractId", "this")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryDummyCommit, 114, Noise, "{0} disk flush: LSN = {1} last LSN = {2} priority = {3}", "prId", "lsn", "lastLsn", "priority")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryUpdateLsn, 115, Noise, "{0} LSN = {1}", "prId", "lsn")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryCommitFast, 116, Noise, "{0} LSN = {1} cost = {2} ms", "prId", "lsn", "cost")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryCommitSlow, 117, Info, "{0} LSN = {1} cost = {2} ms", "prId", "lsn", "cost")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryCommitReadOnly, 118, Noise, "{0}", "ractId")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplicating, 119, Noise, "{0} op-count = {1} buf-count = {2} block = {3} bytes = {4} activity Id = {5} serialization = {6}ms", "ractId", "opcount", "bufcount", "blksize", "bytes", "actId", "serialization")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplicationComplete, 120, Noise, "{0} error = {1} sync = {2}", "ractId", "error", "sync")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplicationTimeout, 121, Noise, "{0} lsn = {1}", "ractId", "lsn")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimarySkipFlush, 122, Noise, "{0} skipping flush: LSN = {1}", "prId", "lsn")
        REPLICATED_STORE_STRUCTURED_TRACE( PrimaryFlushCommits, 123, Noise, "{0} flushing: [{1}, {2}] LSN = {3}", "prId", "start", "end", "lsn")

        REPLICATED_STORE_STRUCTURED_TRACE( NotifyReplication, 130, Noise, "{0} items = {1} lsn = {2}", "prId", "items", "lsn")

        END_REPLSTORE_STRUCTURED_TRACES

        DECLARE_STATE_MACHINE_STRUCTURED_TRACE( QueueEvent, Store::PartitionedReplicaId, ReplicatedStoreEvent::Trace )
        DECLARE_STATE_MACHINE_STRUCTURED_TRACE( ProcessEvent, Store::PartitionedReplicaId, ReplicatedStoreEvent::Trace )
        DECLARE_STATE_MACHINE_STRUCTURED_TRACE( StateChangeInfo, Store::PartitionedReplicaId, ReplicatedStoreState::Trace, ReplicatedStoreState::Trace )
        DECLARE_STATE_MACHINE_STRUCTURED_TRACE( StateChangeNoise, Store::PartitionedReplicaId, ReplicatedStoreState::Trace, ReplicatedStoreState::Trace )
        DECLARE_STATE_MACHINE_STRUCTURED_TRACE( IncrementTransaction, Store::PartitionedReplicaId, int )
        DECLARE_STATE_MACHINE_STRUCTURED_TRACE( DecrementTransaction, Store::PartitionedReplicaId, int )

        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( OperationComplete, Store::ReplicaActivityId, LONGLONG, LONGLONG )

        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryDummyCommit, Store::PartitionedReplicaId, LONGLONG, LONGLONG )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpOperation, Store::PartitionedReplicaId )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpedCopy, Store::PartitionedReplicaId, LONGLONG, uint64, uint64, bool, CopyType::Trace )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpedReplication, Store::PartitionedReplicaId, LONGLONG, LONGLONG, uint64, uint64, bool, Common::Guid )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryApply, Store::PartitionedReplicaId, ReplicationOperationType::Trace, std::wstring, std::wstring, LONGLONG, LONGLONG )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryCommitFast, Store::PartitionedReplicaId, LONGLONG, int64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryCommitSlow, Store::PartitionedReplicaId, LONGLONG, int64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpCtor, uint64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( SecondaryPumpDtor, uint64 )

        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryComReplAsyncCtor, uint64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryComReplAsyncDtor, uint64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplAsyncCtor, Store::ReplicaActivityId, uint64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplAsyncDtor, Store::ReplicaActivityId, uint64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryDummyCommit, Store::PartitionedReplicaId, LONGLONG, LONGLONG, LONGLONG )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryUpdateLsn, Store::PartitionedReplicaId, LONGLONG )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryCommitFast, Store::PartitionedReplicaId, LONGLONG, int64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryCommitSlow, Store::PartitionedReplicaId, LONGLONG, int64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryCommitReadOnly, Store::ReplicaActivityId )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplicating, Store::ReplicaActivityId, uint64, uint64, uint64, uint64, Common::Guid, uint64 )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplicationComplete, Store::ReplicaActivityId, Common::ErrorCodeValue::Trace, bool )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryReplicationTimeout, Store::ReplicaActivityId, LONGLONG )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimarySkipFlush, Store::PartitionedReplicaId, LONGLONG )
        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( PrimaryFlushCommits, Store::PartitionedReplicaId, LONGLONG, LONGLONG, LONGLONG )

        DECLARE_REPLICATED_STORE_STRUCTURED_TRACE( NotifyReplication, Store::PartitionedReplicaId, uint64, LONGLONG )

        static Common::Global<ReplicatedStoreEventSource> Trace;
    };
}
