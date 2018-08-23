// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CommonConfig : ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(CommonConfig, "Common")

        // Performance monitoring interval. Setting to 0 or negative value disables monitoring
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Common", PerfMonitorInterval, Common::TimeSpan::FromSeconds(1), ConfigEntryUpgradePolicy::Dynamic);
        // Performance monitoring trace interval, specified as multiple of PerfMonitorInterval, setting to 0 disables such tracing
        INTERNAL_CONFIG_ENTRY(uint, L"Common", PerfMonitorTraceRatio, 1, ConfigEntryUpgradePolicy::Dynamic);
        // Minimum buffer allocated for reading resource strings
        INTERNAL_CONFIG_ENTRY(int, L"Common", MinResourceStringBufferSizeInWChars, 256, ConfigEntryUpgradePolicy::Static);
        // Maximum buffer allocated for reading resource strings
        INTERNAL_CONFIG_ENTRY(int, L"Common", MaxResourceStringBufferSizeInWChars, 32768, ConfigEntryUpgradePolicy::Static);
        //Maximum allowed Naming URI length including the scheme (depends on local store limits)
        INTERNAL_CONFIG_ENTRY(int, L"Common", MaxNamingUriLength, 512, ConfigEntryUpgradePolicy::Static);

        // The timeout buffer used to send back partial reply when processing times out
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Common", SendReplyBufferTimeout, Common::TimeSpan::FromSeconds(2.0), ConfigEntryUpgradePolicy::Dynamic);

        // The locks to enable in lock traces
        INTERNAL_CONFIG_ENTRY(std::string, L"Common", LockTraceNamePrefix, "", ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(bool, L"Common", MultipleAsyncCallbackInvokeAssertEnabled, true, ConfigEntryUpgradePolicy::Dynamic);

        // -------------------------------
        // HealthManager/ClusterHealthPolicy
        // -------------------------------

        // Cluster health evaluation policy: enable per application type health evaluation
        PUBLIC_CONFIG_ENTRY(bool, L"HealthManager", EnableApplicationTypeHealthEvaluation, false, Common::ConfigEntryUpgradePolicy::Static);

#ifdef PLATFORM_UNIX

        // Timer dispose delay in finalizer queue, should be large enough to ensure timer signal notification be 
        // read by SignalPipeLoop and timer shared_ptr be captured by threadpool post and kept alive for callback
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Common", TimerDisposeDelay, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Common", ProcessExitCacheAgeLimit, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        INTERNAL_CONFIG_ENTRY(uint, L"Common", ProcessExitCacheSizeLimit, 1024*1024, Common::ConfigEntryUpgradePolicy::Static, Common::UIntGreaterThan(0));

        // Where to store mutex, lock and other objects, default to "", meaning $HOME/.service.fabric
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Common", ObjectFolder, L"", ConfigEntryUpgradePolicy::Static);

        // Threshold for sharing POSIX timer, to avoid POSIX timer exhaustion
        INTERNAL_CONFIG_ENTRY(uint, L"Common", PosixTimerLimit, 3000, ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(uint, L"Common", PosixTimerLimit_Fabric, 8000, ConfigEntryUpgradePolicy::Dynamic);

        // Bit count of capacity for timer finalizer queue, with the default value, queue capacity is 2M
        INTERNAL_CONFIG_ENTRY(uint, L"Common", TimerFinalizerQueueCapacityBitCount, 21, ConfigEntryUpgradePolicy::Static, Common::InRange<uint>(10, 32));

        // Dispatch time threshold for TimerQueue timer, longer dispatch time will be traced out
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Common", TimerQueueDispatchTimeThreshold, Common::TimeSpan::FromSeconds(0.1), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanGreaterThan(Common::TimeSpan::Zero));

        // Count of concurrent event loops for sockets, linux only, default to 0 to use processor current. 
        DEPRECATED_CONFIG_ENTRY(uint, L"Common", EventLoopConcurrency, 0, Common::ConfigEntryUpgradePolicy::Static);
        // Cleanup delay for fd context used in event loop
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Common", EventLoopCleanupDelay, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanGreaterThan(Common::TimeSpan::Zero));
#endif
        // Switch to support upgrade and downgrade scenarios without trace loss for transitioning into Structured traces in linux using config upgrade
        // TODO - remove after transition to structured traces is complete
        INTERNAL_CONFIG_ENTRY(bool, L"Common", LinuxStructuredTracesEnabled, false, Common::ConfigEntryUpgradePolicy::Static);


        // See 7268869 and 7278671.  Prior to this, some managed system services did not secure their replicator endpoints.  This config is used to determine whether to read in cluster
        // security settings and pass them into the replicator.  For existing clusters, this config is false and can never be changed.  For new clusters, this should be explicitly set to true so the aforementioned services
        // will read cluster security settings and have their replicators will consume them.
        INTERNAL_CONFIG_ENTRY(bool, L"Common", EnableEndpointV2, false, ConfigEntryUpgradePolicy::NotAllowed);

        // Most public APIs parsing string input restrict the incoming string size to ParameterValidator::MaxStringSize (4K).
        // Certain APIs are allowed to accept longer strings using this config.
        INTERNAL_CONFIG_ENTRY(int, L"Common", MaxLongStringSize, 1024 * 1024, ConfigEntryUpgradePolicy::Dynamic);

        // FabricTest support to not use global ktl system to simplify leak detection
        // By default we will always use a global ktl system in production
        // Used in KtlLoggerNode.cpp and ApplicationHost.cpp
        TEST_CONFIG_ENTRY(bool, L"Common", UseGlobalKtlSystem, true, Common::ConfigEntryUpgradePolicy::Static);
        TEST_CONFIG_ENTRY(uint, L"Common", KtlSystemThreadLimit, 0, Common::ConfigEntryUpgradePolicy::Static);

        // Enables reference tracking to help debug FabricNode leaks in FabricTest random runs
        TEST_CONFIG_ENTRY(bool, L"Common", EnableReferenceTracking, false, ConfigEntryUpgradePolicy::Static);

        // Timeout used for ComponentRoot leak detection timer
        TEST_CONFIG_ENTRY(TimeSpan, L"Common", LeakDetectionTimeout, TimeSpan::FromSeconds(60), ConfigEntryUpgradePolicy::Static);

        // Enables features that are in preview and not supported in production.
        INTERNAL_CONFIG_ENTRY(bool, L"Common", EnableUnsupportedPreviewFeatures, false, ConfigEntryUpgradePolicy::NotAllowed);
    };
}
