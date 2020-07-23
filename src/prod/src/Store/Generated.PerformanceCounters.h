// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once






























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

            COUNTER_DEFINITION( 50+1, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Transactions/sec", L"Transactions created per second" )
            COUNTER_DEFINITION( 50+2, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Inserts/sec", L"Insert operations per second" )
            COUNTER_DEFINITION( 50+3, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Updates/sec", L"Update operations per second" )
            COUNTER_DEFINITION( 50+4, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Deletes/sec", L"Delete operations per second" )
            COUNTER_DEFINITION( 50+5, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Sync Commits/sec", L"Sync committed transactions per second" )
            COUNTER_DEFINITION( 50+6, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Async Commits/sec", L"Async Committed transactions per second" )
            COUNTER_DEFINITION( 50+7, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Rollbacks/sec", L"Rolled back transactions per second" )
            COUNTER_DEFINITION( 50+8, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Enumerations/sec", L"Enumerations created per second" )
            COUNTER_DEFINITION( 50+9, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Reads/sec (type)", L"Type reads per second" )
            COUNTER_DEFINITION( 50+10, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Reads/sec (key)", L"Key reads per second" )
            COUNTER_DEFINITION( 50+11, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Reads/sec (value)", L"Value reads per second" )

            COUNTER_DEFINITION( 100+1, Common::PerformanceCounterType::AverageBase, L"Base for Avg. commit latency (us)", L"", noDisplay)
            COUNTER_DEFINITION( 100+2, Common::PerformanceCounterType::AverageBase, L"Base for Avg. write value size (bytes)", L"", noDisplay)
            COUNTER_DEFINITION( 100+3, Common::PerformanceCounterType::AverageBase, L"Base for Avg. read value size (bytes)", L"", noDisplay)
            COUNTER_DEFINITION( 100+4, Common::PerformanceCounterType::AverageBase, L"Base for Avg. transaction lifetime (ms)", L"", noDisplay)
            COUNTER_DEFINITION( 100+5, Common::PerformanceCounterType::AverageBase, L"Base for Avg. completed commit batch size", L"", noDisplay)
            COUNTER_DEFINITION( 100+6, Common::PerformanceCounterType::AverageBase, L"Base for Avg. completed commit callback duration (us)", L"", noDisplay)

            COUNTER_DEFINITION_WITH_BASE( 150+1, 100+1, Common::PerformanceCounterType::AverageCount64, L"Avg. commit latency (us)", L"Average time to commit a transaction in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 150+2, 100+2, Common::PerformanceCounterType::AverageCount64, L"Avg. write size (bytes)", L"Average size of write (Insert/Update) values in bytes" )
            COUNTER_DEFINITION_WITH_BASE( 150+3, 100+3, Common::PerformanceCounterType::AverageCount64, L"Avg. read size (bytes)", L"Average size of read values in bytes" )
            COUNTER_DEFINITION_WITH_BASE( 150+4, 100+4, Common::PerformanceCounterType::AverageCount64, L"Avg. transaction lifetime (ms)", L"Average lifetime of open transactions in milliseconds" )
            COUNTER_DEFINITION_WITH_BASE( 150+5, 100+5, Common::PerformanceCounterType::AverageCount64, L"Avg. completed commit batch size", L"Average number of commits in each completed commit batch" )
            COUNTER_DEFINITION_WITH_BASE( 150+6, 100+6, Common::PerformanceCounterType::AverageCount64, L"Avg. completed commit callback duration (us)", L"Average time spent in commit completion callback" )
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
            DEFINE_COUNTER_INSTANCE( RateOfTransactions, 50+1 )
            DEFINE_COUNTER_INSTANCE( RateOfInserts, 50+2 )
            DEFINE_COUNTER_INSTANCE( RateOfUpdates, 50+3 )
            DEFINE_COUNTER_INSTANCE( RateOfDeletes, 50+4 )
            DEFINE_COUNTER_INSTANCE( RateOfSyncCommits, 50+5 )
            DEFINE_COUNTER_INSTANCE( RateOfAsyncCommits, 50+6 )
            DEFINE_COUNTER_INSTANCE( RateOfRollbacks, 50+7 )
            DEFINE_COUNTER_INSTANCE( RateOfEnumerations, 50+8 )
            DEFINE_COUNTER_INSTANCE( RateOfTypeReads, 50+9 )
            DEFINE_COUNTER_INSTANCE( RateOfKeyReads, 50+10 )
            DEFINE_COUNTER_INSTANCE( RateOfValueReads, 50+11 )

            DEFINE_COUNTER_INSTANCE( AvgLatencyOfCommitsBase, 100+1 )
            DEFINE_COUNTER_INSTANCE( AvgSizeOfWritesBase, 100+2 )
            DEFINE_COUNTER_INSTANCE( AvgSizeOfReadsBase, 100+3 )
            DEFINE_COUNTER_INSTANCE( AvgTransactionLifetimeBase, 100+4 )
            DEFINE_COUNTER_INSTANCE( AvgCompletedCommitBatchSizeBase, 100+5 )
            DEFINE_COUNTER_INSTANCE( AvgCompletedCommitCallbackDurationBase, 100+6 )

            DEFINE_COUNTER_INSTANCE( AvgLatencyOfCommits, 150+1 )
            DEFINE_COUNTER_INSTANCE( AvgSizeOfWrites, 150+2 )
            DEFINE_COUNTER_INSTANCE( AvgSizeOfReads, 150+3 )
            DEFINE_COUNTER_INSTANCE( AvgTransactionLifetime, 150+4 )
            DEFINE_COUNTER_INSTANCE( AvgCompletedCommitBatchSize, 150+5 )
            DEFINE_COUNTER_INSTANCE( AvgCompletedCommitCallbackDuration, 150+6 )
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

            COUNTER_DEFINITION( 0+1, Common::PerformanceCounterType::RawData64, L"Tombstone Count", L"Number of tombstone entries" )
            COUNTER_DEFINITION( 0+2, Common::PerformanceCounterType::RawData64, L"Notification Dispatch Queue Size", L"Number of replication operations pending dispatch in the notification queue" )

            COUNTER_DEFINITION( 50+1, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Replication operations/sec", L"Replication operations sent per second" )
            COUNTER_DEFINITION( 50+2, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Copy operation reads/sec", L"Copy operations read per second" )
            COUNTER_DEFINITION( 50+3, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Pumped secondary operations/sec", L"Pumped copy and replication operations per second" )

            COUNTER_DEFINITION( 100+1, Common::PerformanceCounterType::AverageBase, L"Base for Avg. commit latency (us)", L"", noDisplay)
            COUNTER_DEFINITION( 100+2, Common::PerformanceCounterType::AverageBase, L"Base for Avg. replication latency (us)", L"", noDisplay)
            COUNTER_DEFINITION( 100+3, Common::PerformanceCounterType::AverageBase, L"Base for Avg. tombstone cleanup latency (ms)", L"", noDisplay)
            COUNTER_DEFINITION( 100+4, Common::PerformanceCounterType::AverageBase, L"Base for Avg. number of keys per transaction", L"", noDisplay)
            COUNTER_DEFINITION( 100+5, Common::PerformanceCounterType::AverageBase, L"Base for Average time to create a copy operation", L"", noDisplay)
            COUNTER_DEFINITION( 100+6, Common::PerformanceCounterType::AverageBase, L"Base for Average size of a copy operation", L"", noDisplay)
            COUNTER_DEFINITION( 100+7, Common::PerformanceCounterType::AverageBase, L"Base for Average time to apply a copy operation", L"", noDisplay)
            COUNTER_DEFINITION( 100+8, Common::PerformanceCounterType::AverageBase, L"Base for Average time to apply a replication operation", L"", noDisplay)

            COUNTER_DEFINITION_WITH_BASE( 150+1, 100+1, Common::PerformanceCounterType::AverageCount64, L"Avg. commit latency (us)", L"Average time to commit a transaction in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 150+2, 100+2, Common::PerformanceCounterType::AverageCount64, L"Avg. replication latency (us)", L"Average time to replicate a transaction in microseconds" )
            COUNTER_DEFINITION_WITH_BASE( 150+3, 100+3, Common::PerformanceCounterType::AverageCount64, L"Avg. tombstone cleanup latency (ms)", L"Average time to cleanup tombstones in milliseconds" )
            COUNTER_DEFINITION_WITH_BASE( 150+4, 100+4, Common::PerformanceCounterType::AverageCount64, L"Avg. number of keys per transaction", L"Average number of keys modified per transaction" )
            COUNTER_DEFINITION_WITH_BASE( 150+5, 100+5, Common::PerformanceCounterType::AverageCount64, L"Avg. copy creation latency (ms)", L"Average time to create a copy operation" )
            COUNTER_DEFINITION_WITH_BASE( 150+6, 100+6, Common::PerformanceCounterType::AverageCount64, L"Avg. copy size (bytes)", L"Average size of a copy operation" )
            COUNTER_DEFINITION_WITH_BASE( 150+7, 100+7, Common::PerformanceCounterType::AverageCount64, L"Avg. copy apply latency (ms)", L"Average time to apply a copy operation" )
            COUNTER_DEFINITION_WITH_BASE( 150+8, 100+8, Common::PerformanceCounterType::AverageCount64, L"Avg. replication apply latency (ms)", L"Average time to apply a replication operation" )
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
            DEFINE_COUNTER_INSTANCE( TombstoneCount, 0+1 )
            DEFINE_COUNTER_INSTANCE( NotificationQueueSize, 0+2 )

            DEFINE_COUNTER_INSTANCE( RateOfReplication, 50+1 )
            DEFINE_COUNTER_INSTANCE( RateOfCopy, 50+2 )
            DEFINE_COUNTER_INSTANCE( RateOfPump, 50+3 )

            DEFINE_COUNTER_INSTANCE( AvgLatencyOfCommitBase, 100+1 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfReplicationBase, 100+2 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfTombstoneCleanupBase, 100+3 )
            DEFINE_COUNTER_INSTANCE( AvgNumberOfKeysBase, 100+4 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfCopyCreateBase, 100+5 )
            DEFINE_COUNTER_INSTANCE( AvgSizeOfCopyBase, 100+6 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfApplyCopyBase, 100+7 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfApplyReplicationBase, 100+8 )

            DEFINE_COUNTER_INSTANCE( AvgLatencyOfCommit, 150+1 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfReplication, 150+2 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfTombstoneCleanup, 150+3 )
            DEFINE_COUNTER_INSTANCE( AvgNumberOfKeys, 150+4 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfCopyCreate, 150+5 )
            DEFINE_COUNTER_INSTANCE( AvgSizeOfCopy, 150+6 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfApplyCopy, 150+7 )
            DEFINE_COUNTER_INSTANCE( AvgLatencyOfApplyReplication, 150+8 )
        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<EseStorePerformanceCounters> EseStorePerformanceCountersSPtr;
    typedef std::shared_ptr<ReplicatedStorePerformanceCounters> ReplicatedStorePerformanceCountersSPtr;
}
