// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define S_RAW_INDEX 0
#define S_RATE_INDEX 50
#define S_AVG_BASE_INDEX 100
#define S_AVG_INDEX 150

#define S_RAW_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION( S_RAW_INDEX+Id, Common::PerformanceCounterType::RawData64, Name, Description )

#define S_RATE_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION( S_RATE_INDEX+Id, Common::PerformanceCounterType::RateOfCountPerSecond32, Name, Description )

#define S_AVG_BASE( Id, Name ) \
    COUNTER_DEFINITION( S_AVG_BASE_INDEX+Id, Common::PerformanceCounterType::AverageBase, Name, L"", noDisplay)

#define S_AVG_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION_WITH_BASE( S_AVG_INDEX+Id, S_AVG_BASE_INDEX+Id, Common::PerformanceCounterType::AverageCount64, Name, Description )

#define S_DEFINE_RAW_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, S_RAW_INDEX+Id )

#define S_DEFINE_RATE_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, S_RATE_INDEX+Id )

#define S_DEFINE_AVG_BASE_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, S_AVG_BASE_INDEX+Id )

#define S_DEFINE_AVG_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, S_AVG_INDEX+Id )

namespace Store
{
    // Generated.PerformanceCounters.h is scraped by a perf counter manifest generation script and 
    // is more verbose to facilitate that generation. Generated.PerformanceCounters.h is generated
    // on build from this file, which is more concise and less error prone when actually defining 
    // the perfcounters.
    //
    class EseStorePerformanceCounters
    {
        DENY_COPY(EseStorePerformanceCounters)

    public:
    
        BEGIN_COUNTER_SET_DEFINITION(
            L"6F7D1505-ADD6-419A-B14D-C726D5392084",
            L"ESE Local Store",
            L"Counters for ESE Local Store",
            Common::PerformanceCounterSetInstanceType::Multiple)

            S_RATE_COUNTER( 1, L"Transactions/sec", L"Transactions created per second" )
            S_RATE_COUNTER( 2, L"Inserts/sec", L"Insert operations per second" )
            S_RATE_COUNTER( 3, L"Updates/sec", L"Update operations per second" )
            S_RATE_COUNTER( 4, L"Deletes/sec", L"Delete operations per second" )
            S_RATE_COUNTER( 5, L"Sync Commits/sec", L"Sync committed transactions per second" )
            S_RATE_COUNTER( 6, L"Async Commits/sec", L"Async Committed transactions per second" )
            S_RATE_COUNTER( 7, L"Rollbacks/sec", L"Rolled back transactions per second" )
            S_RATE_COUNTER( 8, L"Enumerations/sec", L"Enumerations created per second" )
            S_RATE_COUNTER( 9, L"Reads/sec (type)", L"Type reads per second" )
            S_RATE_COUNTER( 10, L"Reads/sec (key)", L"Key reads per second" )
            S_RATE_COUNTER( 11, L"Reads/sec (value)", L"Value reads per second" )

            S_AVG_BASE( 1, L"Base for Avg. commit latency (us)" )
            S_AVG_BASE( 2, L"Base for Avg. write value size (bytes)" )
            S_AVG_BASE( 3, L"Base for Avg. read value size (bytes)" )
            S_AVG_BASE( 4, L"Base for Avg. transaction lifetime (ms)" )
            S_AVG_BASE( 5, L"Base for Avg. completed commit batch size" )
            S_AVG_BASE( 6, L"Base for Avg. completed commit callback duration (us)" )

            S_AVG_COUNTER( 1, L"Avg. commit latency (us)", L"Average time to commit a transaction in microseconds" )
            S_AVG_COUNTER( 2, L"Avg. write size (bytes)", L"Average size of write (Insert/Update) values in bytes" )
            S_AVG_COUNTER( 3, L"Avg. read size (bytes)", L"Average size of read values in bytes" )
            S_AVG_COUNTER( 4, L"Avg. transaction lifetime (ms)", L"Average lifetime of open transactions in milliseconds" )
            S_AVG_COUNTER( 5, L"Avg. completed commit batch size", L"Average number of commits in each completed commit batch" )
            S_AVG_COUNTER( 6, L"Avg. completed commit callback duration (us)", L"Average time spent in commit completion callback" )
        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE( RateOfTransactions )
        DECLARE_COUNTER_INSTANCE( RateOfInserts )
        DECLARE_COUNTER_INSTANCE( RateOfUpdates )
        DECLARE_COUNTER_INSTANCE( RateOfDeletes )
        DECLARE_COUNTER_INSTANCE( RateOfSyncCommits )
        DECLARE_COUNTER_INSTANCE( RateOfAsyncCommits )
        DECLARE_COUNTER_INSTANCE( RateOfRollbacks )
        DECLARE_COUNTER_INSTANCE( RateOfEnumerations )
        DECLARE_COUNTER_INSTANCE( RateOfTypeReads )
        DECLARE_COUNTER_INSTANCE( RateOfKeyReads )
        DECLARE_COUNTER_INSTANCE( RateOfValueReads )

        DECLARE_COUNTER_INSTANCE( AvgLatencyOfCommitsBase )
        DECLARE_COUNTER_INSTANCE( AvgSizeOfWritesBase )
        DECLARE_COUNTER_INSTANCE( AvgSizeOfReadsBase )
        DECLARE_COUNTER_INSTANCE( AvgTransactionLifetimeBase )
        DECLARE_COUNTER_INSTANCE( AvgCompletedCommitBatchSizeBase )
        DECLARE_COUNTER_INSTANCE( AvgCompletedCommitCallbackDurationBase )

        DECLARE_COUNTER_INSTANCE( AvgLatencyOfCommits )
        DECLARE_COUNTER_INSTANCE( AvgSizeOfWrites )
        DECLARE_COUNTER_INSTANCE( AvgSizeOfReads )
        DECLARE_COUNTER_INSTANCE( AvgTransactionLifetime )
        DECLARE_COUNTER_INSTANCE( AvgCompletedCommitBatchSize)
        DECLARE_COUNTER_INSTANCE( AvgCompletedCommitCallbackDuration )

        BEGIN_COUNTER_SET_INSTANCE(EseStorePerformanceCounters)
            S_DEFINE_RATE_COUNTER( 1, RateOfTransactions )
            S_DEFINE_RATE_COUNTER( 2, RateOfInserts )
            S_DEFINE_RATE_COUNTER( 3, RateOfUpdates )
            S_DEFINE_RATE_COUNTER( 4, RateOfDeletes )
            S_DEFINE_RATE_COUNTER( 5, RateOfSyncCommits )
            S_DEFINE_RATE_COUNTER( 6, RateOfAsyncCommits )
            S_DEFINE_RATE_COUNTER( 7, RateOfRollbacks )
            S_DEFINE_RATE_COUNTER( 8, RateOfEnumerations )
            S_DEFINE_RATE_COUNTER( 9, RateOfTypeReads )
            S_DEFINE_RATE_COUNTER( 10, RateOfKeyReads )
            S_DEFINE_RATE_COUNTER( 11, RateOfValueReads )

            S_DEFINE_AVG_BASE_COUNTER( 1, AvgLatencyOfCommitsBase )
            S_DEFINE_AVG_BASE_COUNTER( 2, AvgSizeOfWritesBase )
            S_DEFINE_AVG_BASE_COUNTER( 3, AvgSizeOfReadsBase )
            S_DEFINE_AVG_BASE_COUNTER( 4, AvgTransactionLifetimeBase )
            S_DEFINE_AVG_BASE_COUNTER( 5, AvgCompletedCommitBatchSizeBase )
            S_DEFINE_AVG_BASE_COUNTER( 6, AvgCompletedCommitCallbackDurationBase )

            S_DEFINE_AVG_COUNTER( 1, AvgLatencyOfCommits )
            S_DEFINE_AVG_COUNTER( 2, AvgSizeOfWrites )
            S_DEFINE_AVG_COUNTER( 3, AvgSizeOfReads )
            S_DEFINE_AVG_COUNTER( 4, AvgTransactionLifetime )
            S_DEFINE_AVG_COUNTER( 5, AvgCompletedCommitBatchSize )
            S_DEFINE_AVG_COUNTER( 6, AvgCompletedCommitCallbackDuration )
        END_COUNTER_SET_INSTANCE()
    };

    class ReplicatedStorePerformanceCounters
    {
        DENY_COPY(ReplicatedStorePerformanceCounters)

    public:
    
        BEGIN_COUNTER_SET_DEFINITION(
            L"4078139C-7380-4193-A5C7-FF0EE797A87F",
            L"Replicated Store",
            L"Counters for Replicated Store",
            Common::PerformanceCounterSetInstanceType::Multiple)

            S_RAW_COUNTER( 1, L"Tombstone Count", L"Number of tombstone entries" )
            S_RAW_COUNTER( 2, L"Notification Dispatch Queue Size", L"Number of replication operations pending dispatch in the notification queue" )

            S_RATE_COUNTER( 1, L"Replication operations/sec", L"Replication operations sent per second" )
            S_RATE_COUNTER( 2, L"Copy operation reads/sec", L"Copy operations read per second" )
            S_RATE_COUNTER( 3, L"Pumped secondary operations/sec", L"Pumped copy and replication operations per second" )

            S_AVG_BASE( 1, L"Base for Avg. commit latency (us)" )
            S_AVG_BASE( 2, L"Base for Avg. replication latency (us)" )
            S_AVG_BASE( 3, L"Base for Avg. tombstone cleanup latency (ms)" )
            S_AVG_BASE( 4, L"Base for Avg. number of keys per transaction" )
            S_AVG_BASE( 5, L"Base for Average time to create a copy operation" )
            S_AVG_BASE( 6, L"Base for Average size of a copy operation" )
            S_AVG_BASE( 7, L"Base for Average time to apply a copy operation" )
            S_AVG_BASE( 8, L"Base for Average time to apply a replication operation" )

            S_AVG_COUNTER( 1, L"Avg. commit latency (us)", L"Average time to commit a transaction in microseconds" )
            S_AVG_COUNTER( 2, L"Avg. replication latency (us)", L"Average time to replicate a transaction in microseconds" )
            S_AVG_COUNTER( 3, L"Avg. tombstone cleanup latency (ms)", L"Average time to cleanup tombstones in milliseconds" )
            S_AVG_COUNTER( 4, L"Avg. number of keys per transaction", L"Average number of keys modified per transaction" )
            S_AVG_COUNTER( 5, L"Avg. copy creation latency (ms)", L"Average time to create a copy operation" )
            S_AVG_COUNTER( 6, L"Avg. copy size (bytes)", L"Average size of a copy operation" )
            S_AVG_COUNTER( 7, L"Avg. copy apply latency (ms)", L"Average time to apply a copy operation" )
            S_AVG_COUNTER( 8, L"Avg. replication apply latency (ms)", L"Average time to apply a replication operation" )
        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE( TombstoneCount )
        DECLARE_COUNTER_INSTANCE( NotificationQueueSize )

        DECLARE_COUNTER_INSTANCE( RateOfReplication )
        DECLARE_COUNTER_INSTANCE( RateOfCopy )
        DECLARE_COUNTER_INSTANCE( RateOfPump )

        DECLARE_COUNTER_INSTANCE( AvgLatencyOfCommitBase )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfReplicationBase )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfTombstoneCleanupBase )
        DECLARE_COUNTER_INSTANCE( AvgNumberOfKeysBase )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfCopyCreateBase )
        DECLARE_COUNTER_INSTANCE( AvgSizeOfCopyBase )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfApplyCopyBase )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfApplyReplicationBase )

        DECLARE_COUNTER_INSTANCE( AvgLatencyOfCommit )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfReplication )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfTombstoneCleanup )
        DECLARE_COUNTER_INSTANCE( AvgNumberOfKeys )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfCopyCreate )
        DECLARE_COUNTER_INSTANCE( AvgSizeOfCopy )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfApplyCopy )
        DECLARE_COUNTER_INSTANCE( AvgLatencyOfApplyReplication )

        BEGIN_COUNTER_SET_INSTANCE(ReplicatedStorePerformanceCounters)
            S_DEFINE_RAW_COUNTER( 1, TombstoneCount )
            S_DEFINE_RAW_COUNTER( 2, NotificationQueueSize )

            S_DEFINE_RATE_COUNTER( 1, RateOfReplication )
            S_DEFINE_RATE_COUNTER( 2, RateOfCopy )
            S_DEFINE_RATE_COUNTER( 3, RateOfPump )

            S_DEFINE_AVG_BASE_COUNTER( 1, AvgLatencyOfCommitBase )
            S_DEFINE_AVG_BASE_COUNTER( 2, AvgLatencyOfReplicationBase )
            S_DEFINE_AVG_BASE_COUNTER( 3, AvgLatencyOfTombstoneCleanupBase )
            S_DEFINE_AVG_BASE_COUNTER( 4, AvgNumberOfKeysBase )
            S_DEFINE_AVG_BASE_COUNTER( 5, AvgLatencyOfCopyCreateBase )
            S_DEFINE_AVG_BASE_COUNTER( 6, AvgSizeOfCopyBase )
            S_DEFINE_AVG_BASE_COUNTER( 7, AvgLatencyOfApplyCopyBase )
            S_DEFINE_AVG_BASE_COUNTER( 8, AvgLatencyOfApplyReplicationBase )

            S_DEFINE_AVG_COUNTER( 1, AvgLatencyOfCommit )
            S_DEFINE_AVG_COUNTER( 2, AvgLatencyOfReplication )
            S_DEFINE_AVG_COUNTER( 3, AvgLatencyOfTombstoneCleanup )
            S_DEFINE_AVG_COUNTER( 4, AvgNumberOfKeys )
            S_DEFINE_AVG_COUNTER( 5, AvgLatencyOfCopyCreate )
            S_DEFINE_AVG_COUNTER( 6, AvgSizeOfCopy )
            S_DEFINE_AVG_COUNTER( 7, AvgLatencyOfApplyCopy )
            S_DEFINE_AVG_COUNTER( 8, AvgLatencyOfApplyReplication )
        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<EseStorePerformanceCounters> EseStorePerformanceCountersSPtr;
    typedef std::shared_ptr<ReplicatedStorePerformanceCounters> ReplicatedStorePerformanceCountersSPtr;
}
