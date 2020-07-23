// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace KtlLogger
{
    class KtlLoggerConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(KtlLoggerConfig, "KtlLogger")

        // ---------------   Memory Throttle settings
        
        //The periodic log flush timer when not under memory pressure.
        //This defines the number of seconds that write activity for a
        //log is idle before the cached write data is flushed when the
        //system is not under pressure to flush.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"KtlLogger", PeriodicFlushTime, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
        
        //The periodic log flush timer when under memory pressure. 
        //This defines the number of seconds that write activity for a
        //log is idle before the cached write data is flushed when the
        //system is under pressure to flush.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"KtlLogger", PeriodicTimerInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        
        // Flag that indicates if the memory settings should be automatically and dynamically configured. 
        // If zero then the memory configuration settings are used directly and do not change based on system
        // conditions. If one then the memory settings are configured automatically and may change based on system
        // conditions. 
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", AutomaticMemoryConfiguration, 1, Common::ConfigEntryUpgradePolicy::Static);

        //The timeout for memory allocations. 00:00:00 implies no
        //timeout. This defines the number of seconds that an
        //allocation request for the WriteBufferMemoryPool can wait
        //before being failed.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"KtlLogger", AllocationTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static);
        
        //The number of KB to initially allocate for the write buffer memory pool. Use 0 to indicate no limit 
        //Default should be consistent with SharedLogSizeInMB below
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", WriteBufferMemoryPoolMinimumInKB, 8388608, Common::ConfigEntryUpgradePolicy::Static);
        
        //The number of KB to allow the write buffer memory pool to
        //grow up to. Use 0 to indicate no limit
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", WriteBufferMemoryPoolMaximumInKB, 0, Common::ConfigEntryUpgradePolicy::Static);
        
        //The number of KB to allow the write buffer memory pool to
        //grow for each log stream opened. Use -1 to indicate using default record size
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", WriteBufferMemoryPoolPerStreamInKB, -1, Common::ConfigEntryUpgradePolicy::Static);
        
        //The number of KB to allow for the pinned memory pool. Use 0 to indicate no limit
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", PinnedMemoryLimitInKB, 0, Common::ConfigEntryUpgradePolicy::Static);

        //The number of KB to allow the shared log to advance ahead of
        //the dedicated log. Use 0 to indicate no limit
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", MaximumDestagingWriteOutstandingInKB, 0, Common::ConfigEntryUpgradePolicy::Static);      

        //Default timeout to use for KTL deactivation during abort
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"KtlLogger", KtlDeactivationTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The percentage of usage of the shared log that will induce
        // throttling. Value should be between 0 and 100.
        // A value of 0 implies using the default percentage value.
        // A value of 100 implies no throttling at all.
        // A value between 1 and 99 specifies the percentage of log
        // usage above which throttling will occur, for example if the
        // shared log is 10GB and the value is 90 then throttleing will
        // occur once 9GB is in use.
        PUBLIC_CONFIG_ENTRY(int, L"KtlLogger", SharedLogThrottleLimitInPercentUsed, 0, Common::ConfigEntryUpgradePolicy::Static);

        // These are settings that control the accelerated flush
        // functionality for the shared log. The functionality is in
        // either active mode where the code will try to pick those log
        // streams that are lowest in the log or in passive mode where
        // the code will check if the threshold to enable active mode
        // is reached. The accelerated flush functionality is trigged
        // off a timer where the shared log usage and accelerated
        // flushes may be performed. There is a different timer value
        // for active and passive modes.

        // How often the accelerated flush code will run in active mode
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", AccelerateFlushActiveTimerInMs, 1000, Common::ConfigEntryUpgradePolicy::Static);
        
        // How often the accelerated flush code will run in passive mode
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", AccelerateFlushPassiveTimerInMs, 15000, Common::ConfigEntryUpgradePolicy::Static);
        
        // Percentage of shared log usage above which the functionality
        // will change from passive to active mode.
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", AccelerateFlushActivePercent, 70, Common::ConfigEntryUpgradePolicy::Static);
        
        // Percentage of shared log usage below which the functionality
        // will change from active to passive mode.
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", AccelerateFlushPassivePercent, 30, Common::ConfigEntryUpgradePolicy::Static);


        
        //
        // --------------- Application Shared Log Container settings
        //
        // See prod\src\ServiceModel\ServiceModelConfig.h
        //

        // --------------- System Service Shared Log Container settings 
        //
        // These are currently only used by RA. See prod\src\ServiceModel\data\txnreplicator\SLConfigMacros.h for
        // the config settings used by all other system services.
        //
        
        //Path and file name to location to place shared log container.
        //Use L"" for using default path under fabric data root.
        //(for System Services)
        INTERNAL_CONFIG_ENTRY(std::wstring, L"KtlLogger", SystemSharedLogPath, L"", Common::ConfigEntryUpgradePolicy::Static);

        //The number of MB to allocate in the shared log container (for System Services)
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", SystemSharedLogSizeInMB, 256, Common::ConfigEntryUpgradePolicy::Static);
        
        //The number of log streams to allow in the shared log container (for System Services)
        //
        //The shared log can multiplex multiple log streams. 
        //Each replica uses a single stream in which to store its log data. It may also have a backup stream and a copy stream. 
        //Since the shared log will only have a single replica (RA) then this can be made much smaller but not too small since when we 
        //add functionality to "grow" dedicated logs then there will be multiple streams. The extra overhead for more streams is in 
        //the shared log stream metadata file. This has a 1K or so entry for each stream that sits on disk. The other place is that 
        //when there are more streams the core logger needs to reserve extra space for its checkpoint records.
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", SystemSharedLogNumberStreams, 512, Common::ConfigEntryUpgradePolicy::Static);
        
        //The maximum record size written into the shared log. Use 0 for the default value (for System Services)
        INTERNAL_CONFIG_ENTRY(int, L"KtlLogger", SystemSharedLogMaximumRecordSizeInKB, 0, Common::ConfigEntryUpgradePolicy::Static);
        
        //Shared Log creation flags. Use 0 for non sparse log. (for System Services)
        INTERNAL_CONFIG_ENTRY(uint, L"KtlLogger", SystemSharedLogCreateFlags, 0, Common::ConfigEntryUpgradePolicy::Static);
        
        // For testing purposes to allow running multiple clusters on the same machine concurrently (e.g. multiple FabricTest.exe instances)
        INTERNAL_CONFIG_ENTRY(bool, L"KtlLogger", EnableHashSystemSharedLogIdFromPath, false, Common::ConfigEntryUpgradePolicy::Static);
    };
}
