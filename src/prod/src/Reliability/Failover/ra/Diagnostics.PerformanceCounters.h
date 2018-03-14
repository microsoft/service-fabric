// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class RAPerformanceCounters
            {
                DENY_COPY(RAPerformanceCounters)

            public:
            
                BEGIN_COUNTER_SET_DEFINITION(
                    L"0D00D8E7-7EC5-48D6-979C-EB5A54B78F8C",
                    L"Reconfiguration Agent",
                    L"Counters for Reconfiguration Agent Component",
                    Common::PerformanceCounterSetInstanceType::Multiple)

                    COUNTER_DEFINITION(
                        6,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Completed Upgrades",
                        L"Total number of upgrades that have been completed")

                    COUNTER_DEFINITION(
                        7,
                        Common::PerformanceCounterType::RawData64,
                        L"# Scheduled Entities",
                        L"Number of scheduled entities")

                    COUNTER_DEFINITION(
                        8,
                        Common::PerformanceCounterType::RawData64,
                        L"# Locked Entities",
                        L"Number of locked entities")

                    COUNTER_DEFINITION(
                        9,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Committing Entities",
                        L"Number of entities that are committing")

                    COUNTER_DEFINITION(
                        10,
                        Common::PerformanceCounterType::RawData64,
                        L"# Queued Entities",
                        L"Number of entities waiting for a lock")

                    COUNTER_DEFINITION(
                        11,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Entity Schedule Time ms/Entity Base",
                        L"Base counter for average time for scheduling an entity in milliseconds",
                        noDisplay)

                    COUNTER_DEFINITION_WITH_BASE(
                        12,
                        11,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Entity Schedule Time ms/Entity",
                        L"Average time for scheduling an entity in milliseconds")

                    COUNTER_DEFINITION(
                        13,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Entity Lock Time ms/Entity Base",
                        L"Base counter for average time an entity lock is held in milliseconds",
                        noDisplay)

                    COUNTER_DEFINITION_WITH_BASE(
                        14,
                        13,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Entity Lock Time ms/Entity",
                        L"Average time an entity is locked in milliseconds")

                    COUNTER_DEFINITION(
                        15,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Entity Commit Time ms/Entity Base",
                        L"Base counter for average time an entity took to commit in milliseconds",
                        noDisplay)

                    COUNTER_DEFINITION_WITH_BASE(
                        16,
                        15,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Entity Commit Time ms/Entity",
                        L"Average time an entity is locked in milliseconds")

                    COUNTER_DEFINITION(
                        17,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"Federation Messages/sec",
                        L"Federation Messages received per second")

                    COUNTER_DEFINITION(
                        18,
                        Common::PerformanceCounterType::RawData64,
                        L"# Of Completed Job items",
                        L"Number of Completed job items")

                    COUNTER_DEFINITION(
                        19,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Job Items/Entity Lock Base",
                        L"Base counter for average job items processed in every entity lock",
                        noDisplay)

                    COUNTER_DEFINITION_WITH_BASE(
                        20,
                        19,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Job Items/Entity Lock",
                        L"Average job items processed in every entity lock")

                    COUNTER_DEFINITION(
                        21,
                        Common::PerformanceCounterType::RawData64,
                        L"# FTs",
                        L"Number of Entities in the LFUM")

                    COUNTER_DEFINITION(
                        22,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"Commits/sec",
                        L"Number of commits/sec")

                    COUNTER_DEFINITION(
                        23,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"In-Memory Commits/sec",
                        L"Number of In-Memory Commits/sec")

                    COUNTER_DEFINITION(
                        24,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"Job Items Scheduled/sec",
                        L"Number of Jobitems scheduled/sec")

                    COUNTER_DEFINITION(
                        25,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"Job Items Completed /sec",
                        L"Number of Jobitems completed /sec")

                    COUNTER_DEFINITION(
                        26,
                        Common::PerformanceCounterType::RawData64,
                        L"# Commit Failures",
                        L"Number of Commit Failures")

                    COUNTER_DEFINITION(
                        27,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"# Store Txn/sec",
                        L"Number of store transactions/sec")

                    COUNTER_DEFINITION(
                        28,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"# Store Commit/sec",
                        L"Number of store Commit/sec")

                    COUNTER_DEFINITION(
                        29,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Active Store Txn",
                        L"Number of active store transaction")

                    COUNTER_DEFINITION(
                        30,
                        Common::PerformanceCounterType::RawData64,
                        L"# of committing store txn",
                        L"Number of committing store transaction")

                    COUNTER_DEFINITION(
                        31,
                        Common::PerformanceCounterType::RateOfCountPerSecond32,
                        L"IPC Messages/sec",
                        L"IPC Messages received per second")

                    COUNTER_DEFINITION(
                        32,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Reconfiguring FTs",
                        L"Number of Reconfiguring FTs")

                    COUNTER_DEFINITION(
                        33,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Cleanup Pending FTs",
                        L"Number of Cleanup Pending FTs")

                    COUNTER_DEFINITION(
                        34,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Replica Down Pending FTs",
                        L"Number of Replica Down Pending FTs")

                    COUNTER_DEFINITION(
                        35,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Replica Up Pending FTs",
                        L"Number of Replica Up Pending FTs")

                    COUNTER_DEFINITION(
                        36,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Replica Dropped Pending FTs",
                        L"Number of Replica Dropped Pending FTs")

                    COUNTER_DEFINITION(
                        37,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Replica Open Pending FTs",
                        L"Number of Replica Open Pending FTs")

                    COUNTER_DEFINITION(
                        38,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Replica Close Pending FTs",
                        L"Number of Replica Close Pending FTs")

                    COUNTER_DEFINITION(
                        39,
                        Common::PerformanceCounterType::RawData64,
                        L"# of Service Description Update Pending FTs",
                        L"Number of Service Description Update Pending FTs")

                END_COUNTER_SET_DEFINITION()

                DECLARE_COUNTER_INSTANCE(NumberOfCompletedUpgrades)
                DECLARE_COUNTER_INSTANCE(NumberOfScheduledEntities)
                DECLARE_COUNTER_INSTANCE(NumberOfLockedEntities)
                DECLARE_COUNTER_INSTANCE(NumberOfCommittingEntities)
                DECLARE_COUNTER_INSTANCE(NumberOfQueuedEntities)
                DECLARE_COUNTER_INSTANCE(AverageEntityScheduleTimeBase)
                DECLARE_COUNTER_INSTANCE(AverageEntityScheduleTime)
                DECLARE_COUNTER_INSTANCE(AverageEntityLockTimeBase)
                DECLARE_COUNTER_INSTANCE(AverageEntityLockTime)
                DECLARE_COUNTER_INSTANCE(AverageEntityCommitTimeBase)
                DECLARE_COUNTER_INSTANCE(AverageEntityCommitTime)
                DECLARE_COUNTER_INSTANCE(FederationMessagesReceivedPerSecond)
                DECLARE_COUNTER_INSTANCE(NumberOfCompletedJobItems)
                DECLARE_COUNTER_INSTANCE(AverageJobItemsPerEntityLockBase)
                DECLARE_COUNTER_INSTANCE(AverageJobItemsPerEntityLock)
                DECLARE_COUNTER_INSTANCE(NumberOfEntitiesInLFUM)
                DECLARE_COUNTER_INSTANCE(CommitsPerSecond)
                DECLARE_COUNTER_INSTANCE(InMemoryCommitsPerSecond)
                DECLARE_COUNTER_INSTANCE(JobItemsScheduledPerSecond)
                DECLARE_COUNTER_INSTANCE(JobItemsCompletedPerSecond)
                DECLARE_COUNTER_INSTANCE(NumberOfCommitFailures)
                DECLARE_COUNTER_INSTANCE(NumberOfStoreTransactionsPerSecond)
                DECLARE_COUNTER_INSTANCE(NumberOfStoreCommitsPerSecond)
                DECLARE_COUNTER_INSTANCE(NumberOfActiveStoreTransactions)
                DECLARE_COUNTER_INSTANCE(NumberOfCommittingStoreTransactions)
                DECLARE_COUNTER_INSTANCE(IpcMessagesReceivedPerSecond)
                DECLARE_COUNTER_INSTANCE(NumberOfReconfiguringFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfCleanupPendingFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfReplicaDownPendingFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfReplicaUpPendingFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfReplicaDroppedPendingFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfReplicaOpenPendingFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfReplicaClosePendingFTs)
                DECLARE_COUNTER_INSTANCE(NumberOfServiceDescriptionUpdatePendingFTs)

                BEGIN_COUNTER_SET_INSTANCE(RAPerformanceCounters)
                    DEFINE_COUNTER_INSTANCE(NumberOfCompletedUpgrades,                  6)
                    DEFINE_COUNTER_INSTANCE(NumberOfScheduledEntities,                  7)
                    DEFINE_COUNTER_INSTANCE(NumberOfLockedEntities,                     8)
                    DEFINE_COUNTER_INSTANCE(NumberOfCommittingEntities,                 9)
                    DEFINE_COUNTER_INSTANCE(NumberOfQueuedEntities,                     10)
                    DEFINE_COUNTER_INSTANCE(AverageEntityScheduleTimeBase,              11)
                    DEFINE_COUNTER_INSTANCE(AverageEntityScheduleTime,                  12)
                    DEFINE_COUNTER_INSTANCE(AverageEntityLockTimeBase,                  13)
                    DEFINE_COUNTER_INSTANCE(AverageEntityLockTime,                      14)
                    DEFINE_COUNTER_INSTANCE(AverageEntityCommitTimeBase,                15)
                    DEFINE_COUNTER_INSTANCE(AverageEntityCommitTime,                    16)
                    DEFINE_COUNTER_INSTANCE(FederationMessagesReceivedPerSecond,        17)
                    DEFINE_COUNTER_INSTANCE(NumberOfCompletedJobItems,                  18)
                    DEFINE_COUNTER_INSTANCE(AverageJobItemsPerEntityLockBase,           19)
                    DEFINE_COUNTER_INSTANCE(AverageJobItemsPerEntityLock,               20)
                    DEFINE_COUNTER_INSTANCE(NumberOfEntitiesInLFUM,                     21)
                    DEFINE_COUNTER_INSTANCE(CommitsPerSecond,                           22)
                    DEFINE_COUNTER_INSTANCE(InMemoryCommitsPerSecond,                   23)
                    DEFINE_COUNTER_INSTANCE(JobItemsScheduledPerSecond,                 24)
                    DEFINE_COUNTER_INSTANCE(JobItemsCompletedPerSecond,                 25)
                    DEFINE_COUNTER_INSTANCE(NumberOfCommitFailures,                     26)        
                    DEFINE_COUNTER_INSTANCE(NumberOfStoreTransactionsPerSecond,         27)
                    DEFINE_COUNTER_INSTANCE(NumberOfStoreCommitsPerSecond,              28)
                    DEFINE_COUNTER_INSTANCE(NumberOfActiveStoreTransactions,            29)
                    DEFINE_COUNTER_INSTANCE(NumberOfCommittingStoreTransactions,        30)
                    DEFINE_COUNTER_INSTANCE(IpcMessagesReceivedPerSecond,               31)      
                    DEFINE_COUNTER_INSTANCE(NumberOfReconfiguringFTs,                   32)
                    DEFINE_COUNTER_INSTANCE(NumberOfCleanupPendingFTs,                  33)
                    DEFINE_COUNTER_INSTANCE(NumberOfReplicaDownPendingFTs,              34)
                    DEFINE_COUNTER_INSTANCE(NumberOfReplicaUpPendingFTs,                35)
                    DEFINE_COUNTER_INSTANCE(NumberOfReplicaDroppedPendingFTs,           36)
                    DEFINE_COUNTER_INSTANCE(NumberOfReplicaOpenPendingFTs,              37)
                    DEFINE_COUNTER_INSTANCE(NumberOfReplicaClosePendingFTs,             38)
                    DEFINE_COUNTER_INSTANCE(NumberOfServiceDescriptionUpdatePendingFTs, 39)
                END_COUNTER_SET_INSTANCE()

            public:
                void OnUpgradeCompleted();

                void UpdateAverageDurationCounter(
                    Common::StopwatchTime startTime,
                    Common::PerformanceCounterData & base,
                    Common::PerformanceCounterData & counter);

                void UpdateAverageDurationCounter(
                    Common::StopwatchTime startTime,
                    Infrastructure::IClock & clock,
                    Common::PerformanceCounterData & base,
                    Common::PerformanceCounterData & counter);

                void UpdateAverageDurationCounter(
                    Common::TimeSpan elapsed,
                    Common::PerformanceCounterData & base,
                    Common::PerformanceCounterData & counter);
            };
        }
    }
}

